#include "connect.h"

/********************************     连通域     **********************************************************/


static inline bool Touch8(const RunRLE& upper, const RunRLE& lower) {
    const int ax0 = upper.x1 - 1;
    const int ax1 = upper.x2 + 1;
    return !(ax1 < lower.x1 || lower.x2 < ax0);
}

static RegionRLE NormalizeByRow(RegionRLE reg) {
    if (reg.runs.empty()) return reg;

    std::sort(reg.runs.begin(), reg.runs.end(), [](const RunRLE& a, const RunRLE& b) {
        if (a.y != b.y) return a.y < b.y;
        return a.x1 < b.x2;
        });

    std::vector<RunRLE> merged;
    merged.reserve(reg.runs.size());

    for (const auto& r : reg.runs) {
        if (merged.empty() || merged.back().y != r.y || r.x1 > merged.back().x2 + 1) {
            merged.push_back(r);
        }
        else {
            merged.back().x2 = std::max(merged.back().x2, r.x2);
        }
    }

    reg.runs.swap(merged);
    return reg;
}

static std::vector<RowSpan> BuildRowSpans(const std::vector<RunRLE>& runsSorted) {
    std::vector<RowSpan> spans;
    if (runsSorted.empty()) return spans;

    int curY = runsSorted[0].y;
    int begin = 0;

    for (int i = 1; i < (int)runsSorted.size(); ++i) {
        if (runsSorted[i].y != curY) {
            spans.push_back(RowSpan{ curY, begin, i });
            curY = runsSorted[i].y;
            begin = i;
        }
    }
    spans.push_back(RowSpan{ curY, begin, (int)runsSorted.size() });
    return spans;
}

// -------------------- Main: connection (8-neighborhood) --------------------

std::vector<RegionRLE> Connection8(RegionRLE regionIn) {
    // 1) Normalize & sort
    RegionRLE reg = NormalizeByRow(std::move(regionIn));
    if (reg.runs.empty()) return {};

    // runs are now sorted by (y,x0)
    const int n = (int)reg.runs.size();
    DSU dsu(n);

    // 2) Build row spans
    const std::vector<RowSpan> spans = BuildRowSpans(reg.runs);

    // 3) Union runs that touch across adjacent rows
    for (int k = 1; k < (int)spans.size(); ++k) {
        const RowSpan& prev = spans[k - 1];
        const RowSpan& cur = spans[k];

        if (cur.y != prev.y + 1) continue; // only adjacent rows can connect directly

        int i = prev.begin;
        int j = cur.begin;

        // two-pointer scan
        while (i < prev.end && j < cur.end) {
            const RunRLE& ra = reg.runs[i]; // upper row
            const RunRLE& rb = reg.runs[j]; // lower row

            // expand upper interval by 1 for 8-neighborhood, used for fast skipping
            const int ax0 = ra.x1 - 1;
            const int ax1 = ra.x2 + 1;

            if (ax1 < rb.x1) { ++i; continue; } // upper too far left
            if (rb.x2 < ax0) { ++j; continue; } // lower too far left

            // ra may touch multiple runs in current row; scan forward from j
            int jj = j;
            while (jj < cur.end) {
                const RunRLE& rb2 = reg.runs[jj];
                if (rb2.x1 > ax1) break;         // beyond expanded right bound
                if (Touch8(ra, rb2)) dsu.unite(i, jj);
                ++jj;
            }

            // advance side with smaller (actual) x1
            if (ra.x2 < rb.x2) ++i;
            else ++j;
        }
    }

    // 4) Group runs by DSU root
    std::unordered_map<int, std::vector<RunRLE>> groups;
    groups.reserve((size_t)n);

    for (int idx = 0; idx < n; ++idx) {
        groups[dsu.find(idx)].push_back(reg.runs[idx]);
    }

    // 5) Build output components
    std::vector<RegionRLE> out;
    out.reserve(groups.size());

    for (auto& kv : groups) {
        RegionRLE comp;
        comp.runs = std::move(kv.second);
        std::sort(comp.runs.begin(), comp.runs.end(), [](const RunRLE& a, const RunRLE& b) {
            if (a.y != b.y) return a.y < b.y;
            return a.x1 < b.x1;
            });
        out.push_back(std::move(comp));
    }

    return out;
}

static inline bool clip_run(int& xs, int& xe, int width) {
    if (xs > xe) std::swap(xs, xe);
    if (xe <= 0 || xs >= width) return false; // 注意 xe<=0
    xs = std::max(xs, 0);
    xe = std::min(xe, width);                 // 关键：上限是 width（右开）
    return xs < xe;
}

ThreshResultU8 thresholdInRegionU8(const cv::Mat& grayU8, const RegionRLE& reg, uint8_t lo, uint8_t hi) {
    CV_Assert(grayU8.type() == CV_8UC1);
    int64_t area = 0;
    ThreshResultU8 out;

    out.bin = cv::Mat(grayU8.rows, grayU8.cols, CV_8UC1, cv::Scalar(0));

    // 统计外接矩形：min/max
    int minx = INT_MAX, miny = INT_MAX;
    int maxx = -1, maxy = -1;

    for (const auto& r : reg.runs) {
        if (r.y < 0 || r.y >= grayU8.rows) continue;
        int xs = r.x1, xe = r.x2;
        if (!clip_run(xs, xe, grayU8.cols)) continue;

        const uint8_t* src = grayU8.ptr<uint8_t>(r.y);
        uint8_t* dst = out.bin.ptr<uint8_t>(r.y);

        for (int x = xs; x < xe; ++x) {
            uint8_t v = src[x];
            uint8_t m = (v >= lo && v <= hi) ? 255 : 0;

            dst[x] = m;
            area += (m != 0);

            // 更新外接矩形
            if (x < minx) minx = x;
            if (x > maxx) maxx = x;
            if (r.y < miny) miny = r.y;
            if (r.y > maxy) maxy = r.y;

        }
    }
    out.area = area;

    if (area > 0) {
        //out.hasRect = true;

        out.rect = cv::Rect(minx, miny, (maxx - minx + 1), (maxy - miny + 1));
    }
    else {
        //out.hasRect = false;
        out.rect = cv::Rect(); // 空
    }

    return out;
}