#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <numeric>


// =====  行程编码  =====
struct RunRLE {
    int y = 0;
    int x1 = 0;
    int x2 = -1;
};

struct RowSpan {
    int y;
    int begin;
    int end;
};


// ===== Region =====
class RegionRLE {
public:
    int img_w = 0;
    int img_h = 0;

    std::vector<RunRLE> runs;

    int minx, miny, maxx, maxy; // bbox

    int64_t area = 0;

    bool valid = false; //标志位

public:
    RegionRLE(int w = 0, int h = 0) : img_w(w), img_h(h) { resetStats(); }

    void setImageSize(int w, int h) { img_w = w; img_h = h; reset(); }

    void reset() {
        runs.clear();
        resetStats();
    }

    void resetStats(); 

    //行程编码核心函数
    void addRun(int y, int x1, int x2);

    // ---- 合并重合或者相邻的行程编码 ----
    void normalize();

    // ---- 重新计算bbox边界和面积 ----
    void refreshStats();

    // ---- 平移 ----
    void translate(int dx, int dy);

    // ---- 避免region越界 ----
    void clipToImage(); 

    // ---- 判断点是否在region里面 ----
    bool contains(int x, int y); 


};

 /********************************      region 交集、并集、差集、或集 操作     **********************************************************/

// ---- union1 和HALCON union1 相当----
RegionRLE Union1(const std::vector<RegionRLE>& regs);
// ---- union 和halcon的union2效果相当  并集----
RegionRLE Union2(const RegionRLE& A, const RegionRLE& B);
// ---- intersection 交集 ----
RegionRLE Intersection(const RegionRLE& A0, const RegionRLE& B0);
// ---- difference  A - B 差集 ----
RegionRLE Difference(const RegionRLE& A0, const RegionRLE& B0);
// ---- xor  (A - B) ∪ (B - A) ----
RegionRLE Xor(const RegionRLE& A, const RegionRLE& B);


