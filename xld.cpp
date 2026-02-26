#include"xld.h"

static inline std::vector<RowSpan> buildRowSpansFromRuns(const std::vector<RunRLE>& runs) {
    std::vector<RowSpan> spans;
    if (runs.empty()) return spans;

    int begin = 0;
    int curY = runs[0].y;

    for (int i = 1; i <= (int)runs.size(); ++i) {
        if (i == (int)runs.size() || runs[i].y != curY) {
            spans.push_back({ curY, begin, i });
            if (i < (int)runs.size()) {
                begin = i;
                curY = runs[i].y;
            }
        }
    }
    return spans;
}

static inline const RowSpan* findSpan(const std::vector<RowSpan>& spans, int y) {
    auto it = std::lower_bound(spans.begin(), spans.end(), y,
        [](const RowSpan& s, int yy) { return s.y < yy; });
    if (it == spans.end() || it->y != y) return nullptr;
    return &(*it);
}


static std::vector<EdgePoint> regionToEdges_RLE(const RegionRLE& region0)
{
    RegionRLE region = region0;
    if (!region.valid) region.normalize();

    std::vector<EdgePoint> edges;
    if (region.runs.empty()) return edges;

    const auto spans = buildRowSpansFromRuns(region.runs);
    edges.reserve(region.runs.size() * 4);

    auto emitSegH = [&](int y, int x1, int x2_plus1) { // [x1, x2_plus1) on grid
        if (x2_plus1 > x1) edges.push_back({ cv::Point(x1, y), cv::Point(x2_plus1, y) });
    };
    auto emitSegV = [&](int x, int y1, int y2) { // [y1, y2]
        if (y2 > y1) edges.push_back({ cv::Point(x, y1), cv::Point(x, y2) });
    };

    for (const auto& r : region.runs) {
        const int y = r.y;
        const int xs = r.x1;
        const int xe = r.x2;          // pixel inclusive
        const int xr = xe + 1;        // right boundary x on grid

        // 左右边界（每条 run 两端必是边界）
        emitSegV(xs, y, y + 1);
        emitSegV(xr, y, y + 1);

        // ---- 上边界：看 y-1 行哪些区间覆盖了 [xs, xe] ----
        {
            const RowSpan* up = findSpan(spans, y - 1);
            if (!up) {
                // 上一行没有前景：整段是上边界
                emitSegH(y, xs, xr);
            }
            else {
                int curr = xs;
                const auto* arr = region.runs.data();
                for (int i = up->begin; i < up->end; ++i) {
                    const auto& u = arr[i];
                    if (u.x2 < curr) continue;
                    if (u.x1 > xe) break;

                    if (u.x1 > curr) {
                        // gap [curr, u.x1-1] => grid segment [curr, u.x1)
                        emitSegH(y, curr, u.x1);
                    }
                    curr = std::max(curr, u.x2 + 1);
                    if (curr > xe) break;
                }
                if (curr <= xe) {
                    emitSegH(y, curr, xr);
                }
            }
        }

        // ---- 下边界：看 y+1 行覆盖情况 ----
        {
            const RowSpan* dn = findSpan(spans, y + 1);
            if (!dn) {
                emitSegH(y + 1, xs, xr);
            }
            else {
                int curr = xs;
                const auto* arr = region.runs.data();
                for (int i = dn->begin; i < dn->end; ++i) {
                    const auto& d = arr[i];
                    if (d.x2 < curr) continue;
                    if (d.x1 > xe) break;

                    if (d.x1 > curr) {
                        emitSegH(y + 1, curr, d.x1);
                    }
                    curr = std::max(curr, d.x2 + 1);
                    if (curr > xe) break;
                }
                if (curr <= xe) {
                    emitSegH(y + 1, curr, xr);
                }
            }
        }
    }

    return edges;
}

static EdgeGraph buildEdgeGraph(const std::vector<EdgePoint>& rawEdges, int imgWidth, int imgHeight)
{
    EdgeGraph g;
    g.vertices.reserve(rawEdges.size());
    g.edges.reserve(rawEdges.size());

    const int stride = imgWidth + 1;
    const int size = (imgWidth + 1) * (imgHeight + 1);

    std::vector<int> idMap(size, -1);

    auto getVertex = [&](int x, int y) -> int {
        // 防御（可选）
        if (x < 0) x = 0; else if (x > imgWidth)  x = imgWidth;
        if (y < 0) y = 0; else if (y > imgHeight) y = imgHeight;

        const int idx = y * stride + x;
        int& id = idMap[idx];
        if (id != -1) return id;

        id = (int)g.vertices.size();
        g.vertices.emplace_back(cv::Point(x, y));
        return id;
    };

    for (const auto& e : rawEdges) {
        const int v1 = getVertex(e.p1.x, e.p1.y);
        const int v2 = getVertex(e.p2.x, e.p2.y);
        if (v1 == v2) continue;

        const int ei = (int)g.edges.size();
        g.edges.emplace_back(v1, v2);
        g.vertices[v1].incidentEdges.push_back(ei);
        g.vertices[v2].incidentEdges.push_back(ei);
    }

    return g;
}

static inline int otherVertex(const GraphEdge& e, int v) {
    return (e.v1 == v) ? e.v2 : e.v1;
}

std::vector<std::vector<cv::Point>> extractContours(const EdgeGraph& g)
{
    std::vector<std::vector<cv::Point>> contours;
    if (g.edges.empty()) return contours;

    std::vector<char> used(g.edges.size(), 0);

    for (int ei = 0; ei < (int)g.edges.size(); ++ei) {
        if (used[ei]) continue;

        std::vector<cv::Point> contour;
        contour.reserve(256);

        const auto& e0 = g.edges[ei];
        used[ei] = 1;

        int vStart = e0.v1;
        int vCurr = e0.v2;
        int vPrev = vStart;

        contour.push_back(g.vertices[vStart].pt);
        contour.push_back(g.vertices[vCurr].pt);

        while (true) {
            if (vCurr == vStart) break;

            const auto& inc = g.vertices[vCurr].incidentEdges;

            int nextE = -1, nextV = -1;

            // 1) 优先不回头
            for (int ne : inc) {
                if (used[ne]) continue;
                int ov = otherVertex(g.edges[ne], vCurr);
                if (ov == vPrev) continue;
                nextE = ne; nextV = ov;
                break;
            }
            // 2) 兜底：允许回头（避免断链）
            if (nextE < 0) {
                for (int ne : inc) {
                    if (used[ne]) continue;
                    int ov = otherVertex(g.edges[ne], vCurr);
                    nextE = ne; nextV = ov;
                    break;
                }
            }

            if (nextE < 0) break; // 走不下去（非理想拓扑）

            used[nextE] = 1;
            vPrev = vCurr;
            vCurr = nextV;
            contour.push_back(g.vertices[vCurr].pt);

            // 防死循环
            if ((int)contour.size() > (int)g.edges.size() + 5) break;
        }

        // 去掉闭合重复点（可选）
        if (!contour.empty() && contour.front() == contour.back()) {
            contour.pop_back();
        }

        if (contour.size() >= 3) contours.push_back(std::move(contour));
    }

    return contours;
}

std::vector<std::vector<cv::Point>> GenContourXld(const RegionRLE& region)
{
    auto edges = regionToEdges_RLE(region);
    EdgeGraph g = buildEdgeGraph(edges, region.img_w, region.img_h);
    return extractContours(g);
}



std::vector<cv::Point> findsMaxArea(const cv::Mat& src, int& maxArea, cv::Rect& rect)
{
    if (src.empty() || 1 != src.channels())
        return std::vector<cv::Point>();

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(src, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    int contourSize = contours.size();
    if (0 < contourSize)
    {
        int maxIdx = -1;
        std::atomic<int> _maxArea = 0;
        cv::Rect _rect;
        std::mutex _mutex;

        cv::parallel_for_(cv::Range(0, contourSize), [&](const cv::Range& range) {
            //cv::Range range = cv::Range(0, contourSize); // 调试用
            for (auto i = range.start; i < range.end; ++i)
            {
                auto& c = contours[i];
                cv::Rect _r = cv::boundingRect(c);
                int area = _r.area();
                if (1 < area)
                {
                    cv::Mat rect_Img = src(_r);
                    area = cv::countNonZero(rect_Img);
                }

                std::lock_guard<std::mutex> locker(_mutex);
                if (_maxArea < area)
                {
                    _maxArea = area;
                    maxIdx = i;
                    _rect = _r;
                }
            }
            });
        maxArea = _maxArea;
        rect = _rect;
        return contours[maxIdx];
    }

    return std::vector<cv::Point>();
}


std::vector<cv_Contour2D> SelectShapeArea(const cv::Mat& binImage, int mins, int maxs)
{
    if (binImage.empty() || 1 != binImage.channels())
        return std::vector <cv_Contour2D>();

    if (mins > maxs)
    {
        int temp = mins;
        mins = maxs;
        maxs = temp;
    }

    std::vector<std::vector<cv::Point>> contours;
    std::vector <cv_Contour2D> contoursR;
    cv::findContours(binImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int contourSize = contours.size();
    if (0 < contourSize)
    {
        int maxIdx = -1;
        std::atomic<int> _maxArea = 0;
        cv::Rect _rect;
        std::mutex _mutex;

        cv::parallel_for_(cv::Range(0, contourSize), [&](const cv::Range& range) {
            //cv::Range range = cv::Range(0, contourSize); // 调试用
            for (auto i = range.start; i < range.end; ++i)
            {
                auto& c = contours[i];
                cv::Rect _r = cv::boundingRect(c);
                int area = _r.area();
                if (1 < area)
                {
                    cv::Mat rect_Img = binImage(_r);
                    area = cv::countNonZero(rect_Img);
                }

                std::lock_guard<std::mutex> locker(_mutex);
                if (area < mins || area > maxs)
                {
                    continue;
                }
                cv_Contour2D contourR = { c,area,_r };
                contoursR.push_back(contourR);
                //if (_maxArea < area)
                //{
                //	_maxArea = area;
                //	maxIdx = i;
                //	_rect = _r;
                //}
            }
            });
    }
    return contoursR;
}
