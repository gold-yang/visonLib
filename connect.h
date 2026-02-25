#pragma once
#include<opencv2/opencv.hpp>
#include"region.h"

/********************************    connection连通域    **********************************************************/
struct ThreshResultU8 {
    cv::Mat bin;       // 缺陷二值图
    int64_t area = 0;
    cv::Rect rect;
};

struct DSU {
    std::vector<int> parent;
    std::vector<int> rank;
    explicit DSU(int n) : parent(n), rank(n, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }
    int find(int x) {
        return parent[x] == x ? x : (parent[x] = find(parent[x]));
    }
    void unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return;
        if (rank[a] < rank[b]) std::swap(a, b);
        parent[b] = a;
        if (rank[a] == rank[b]) rank[a]++;
    }
};


// static std::vector<RowSpan> BuildRowSpans(const std::vector<RunRLE>& runsSorted);

ThreshResultU8 thresholdInRegionU8(const cv::Mat& grayU8, const RegionRLE& reg, uint8_t lo, uint8_t hi);

std::vector<RegionRLE> Connection8(RegionRLE regionIn);