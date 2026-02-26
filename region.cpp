#include "region.h"

void RegionRLE::resetStats() {
    minx = std::numeric_limits<int>::max();
    miny = std::numeric_limits<int>::max();
    maxx = std::numeric_limits<int>::min();
    maxy = std::numeric_limits<int>::min();
    area = 0;
    valid = false;
}

//行程编码核心函数
void RegionRLE::addRun(int y, int x1, int x2) {
    if (x2 < x1) return;
    if (img_h > 0 && (y < 0 || y >= img_h)) return;

    if (img_w > 0) {
        if (x2 < 0 || x1 >= img_w) return;
        x1 = std::max(x1, 0);
        x2 = std::min(x2, img_w - 1);
        if (x2 < x1) return;
    }
    runs.push_back({ y, x1, x2 });
    valid = false;
}

// ---- 合并重合或者相邻的行程编码 ----
void RegionRLE::normalize() {
    if (runs.empty()) { resetStats(); return; }

    std::sort(runs.begin(), runs.end(), [](const RunRLE& a, const RunRLE& b) {
        if (a.y != b.y) return a.y < b.y;
        return a.x1 < b.x1;
        });

    std::vector<RunRLE> out;
    out.reserve(runs.size());

    RunRLE cur = runs[0];
    for (size_t i = 1; i < runs.size(); ++i) {
        const auto& r = runs[i];
        if (r.y == cur.y && r.x1 <= cur.x2 + 1) {
            cur.x2 = std::max(cur.x2, r.x2);  // 合并
        }
        else {
            out.push_back(cur);
            cur = r;
        }
    }
    out.push_back(cur);
    runs.swap(out);

    refreshStats();
}

// ---- 重新计算bbox边界和面积 ----
void RegionRLE::refreshStats() {
    resetStats();
    if (runs.empty()) return;

    for (const auto& r : runs) {
        miny = std::min(miny, r.y);
        maxy = std::max(maxy, r.y);
        minx = std::min(minx, r.x1);
        maxx = std::max(maxx, r.x2);
        area += int64_t(r.x2 - r.x1 + 1);
    }
    valid = true;
}

// ---- 平移 ----
void RegionRLE::translate(int dx, int dy) {
    if (runs.empty()) return;
    for (auto& r : runs) { r.y += dy; r.x1 += dx; r.x2 += dx; }
    valid = false;
    if (img_w > 0 || img_h > 0) clipToImage();
    else normalize();
}


// ---- 避免region越界 ----
void RegionRLE::clipToImage() {
    if (img_w <= 0 || img_h <= 0) { normalize(); return; }

    std::vector<RunRLE> out;
    out.reserve(runs.size());

    for (auto r : runs) {
        if (r.y < 0 || r.y >= img_h) continue;
        if (r.x2 < 0 || r.x1 >= img_w) continue;
        r.x1 = std::max(r.x1, 0);
        r.x2 = std::min(r.x2, img_w - 1);
        if (r.x2 >= r.x1) out.push_back(r);
    }
    runs.swap(out);
    normalize();
}

// ---- 判断点是否在region里面 ----
bool RegionRLE::contains(int x, int y) {
    if (runs.empty()) return false;

    auto it = std::lower_bound(runs.begin(), runs.end(), y,
        [](const RunRLE& r, int yy) { return r.y < yy; });

    for (; it != runs.end() && it->y == y; ++it) {
        if (x < it->x1) return false;
        if (x <= it->x2) return true;
    }

    return false;
}


/********************************      region 交集、并集、差集、或集 操作     **********************************************************/

// ---- union1 和HALCON union1 相当 ----
 RegionRLE Union1(const std::vector<RegionRLE>& regs) {
    if (regs.empty()) {
        return RegionRLE(0, 0); // empty object tuple -> empty region
    }

    // Determine if all image sizes are consistent (optional meta info)
    int w = 0, h = 0;
    bool haveSize = false;
    bool sameSize = true;

    // pick first valid size as reference
    for (const auto& r : regs) {
        if (r.img_w > 0 && r.img_h > 0) {
            w = r.img_w; h = r.img_h;
            haveSize = true;
            break;
        }
    }
    if (haveSize) {
        for (const auto& r : regs) {
            if (r.img_w > 0 && r.img_h > 0) {
                if (r.img_w != w || r.img_h != h) { sameSize = false; break; }
            }
        }
    }
    else {
        sameSize = false; // no size binding
    }

    RegionRLE out(sameSize ? w : 0, sameSize ? h : 0);

    // reserve total runs to reduce reallocations
    size_t totalRuns = 0;
    for (const auto& r : regs) totalRuns += r.runs.size();
    out.runs.reserve(totalRuns);

    // append all runs (do not assume inputs normalized)
    for (const auto& r : regs) {
        out.runs.insert(out.runs.end(), r.runs.begin(), r.runs.end());
    }

    // canonicalize: sort + merge + stats
    out.normalize();
    return out;
}

// ---- union 和halcon的union2效果相当  并集 ----
 RegionRLE Union2(const RegionRLE& A, const RegionRLE& B) {
    RegionRLE C(A.img_w, A.img_h);
    C.runs.reserve(A.runs.size() + B.runs.size());
    C.runs.insert(C.runs.end(), A.runs.begin(), A.runs.end());
    C.runs.insert(C.runs.end(), B.runs.begin(), B.runs.end());
    C.normalize();
    return C;
}

// ---- intersection 交集 ----
 RegionRLE Intersection(const RegionRLE& A0, const RegionRLE& B0) {
    RegionRLE A = A0, B = B0;
    if (!A.valid) A.normalize();
    if (!B.valid) B.normalize();

    RegionRLE C(A.img_w, A.img_h);

    size_t i = 0, j = 0;
    while (i < A.runs.size() && j < B.runs.size()) {
        const auto& a = A.runs[i];
        const auto& b = B.runs[j];

        if (a.y < b.y) { ++i; continue; }
        if (a.y > b.y) { ++j; continue; }

        // same row: overlap check
        int left = std::max(a.x1, b.x1);
        int right = std::min(a.x2, b.x2);
        if (left <= right) C.runs.push_back({ a.y, left, right });

        // advance smaller-ending run
        if (a.x2 < b.x2) ++i;
        else ++j;
    }

    C.normalize();
    return C;
}

// ---- difference  A - B 差集 ----
 RegionRLE Difference(const RegionRLE& A0, const RegionRLE& B0) {
    RegionRLE A = A0, B = B0;
    if (!A.valid) A.normalize();
    if (!B.valid) B.normalize();

    RegionRLE C(A.img_w, A.img_h);

    if (A.runs.empty()) {
        return C;
    }
    if (B.runs.empty()) {
        C = A;
        return C;
    }

    C.runs.reserve(A.runs.size()); // 粗略预估

    size_t i = 0; // A pointer
    size_t j = 0; // B pointer

    while (i < A.runs.size()) {
        const int y = A.runs[i].y;

        // 把 j 推到第一个 B.y >= 当前 y
        while (j < B.runs.size() && B.runs[j].y < y) ++j;

        // B 在当前 y 的范围 [jb, je)
        size_t jb = j;
        size_t je = jb;
        while (je < B.runs.size() && B.runs[je].y == y) ++je;

        // A 在当前 y 的范围 [i, iRowEnd)
        size_t iRowEnd = i;
        while (iRowEnd < A.runs.size() && A.runs[iRowEnd].y == y) ++iRowEnd;

        // 如果 B 没有这一行，直接拷贝 A 的这一行
        if (jb == je) {
            for (size_t p = i; p < iRowEnd; ++p) C.runs.push_back(A.runs[p]);
            i = iRowEnd;
            continue;
        }

        // 对这一行：逐个 A-run 减去 B-runs
        size_t k = jb; // B 行内游标（单调前进）

        for (size_t p = i; p < iRowEnd; ++p) {
            const auto& a = A.runs[p];
            int cur = a.x1;

            // 跳过所有在 cur 左侧结束的 B-run
            while (k < je && B.runs[k].x2 < cur) ++k;

            size_t kk = k; // 本次 a-run 的 B 游标
            while (kk < je) {
                const auto& b = B.runs[kk];

                if (b.x1 > a.x2) break; // b 起点已在 a 右侧，结束
                if (b.x2 < cur) { ++kk; continue; }

                // 1) cur 到 b.x1-1 的空隙保留
                if (b.x1 > cur) {
                    int left = cur;
                    int right = std::min(a.x2, b.x1 - 1);
                    if (left <= right) C.runs.push_back({ y, left, right });
                }

                // 2) cur 跳到 b 之后
                cur = std::max(cur, b.x2 + 1);
                if (cur > a.x2) break; // a-run 已被完全减掉
                ++kk;
            }

            // 3) 尾巴（最后一个 b 之后）
            if (cur <= a.x2) {
                C.runs.push_back({ y, cur, a.x2 });
            }

            // k 单调前进，给同一行下一个 a-run 复用
            k = kk;
        }

        i = iRowEnd;
        j = jb; // 保持在当前 y 的起点（下一轮会继续推进到 >= 新 y）
    }

    C.refreshStats();
    return C;
}

// ---- xor  (A - B) ∪ (B - A) ----
 RegionRLE Xor(const RegionRLE& A, const RegionRLE& B) {
    RegionRLE D1 = Difference(A, B);
    RegionRLE D2 = Difference(B, A);
    // Union 会 normalize，保证最终 canonical
    return Union2(D1, D2);
}
