
#include"region.h"
#include"image_transform.h"

std::unordered_map<int, RegionRLE>  UCharLabelImageToRegions(const cv::Mat& labelImg)
{
    CV_Assert(labelImg.type() == CV_8UC1);

    const int H = labelImg.rows;
    const int W = labelImg.cols;

    std::unordered_map<int, RegionRLE> regions;
    regions.reserve(32); // 预设label数空间

    for (int y = 0; y < H; ++y) {
        const uchar* row = labelImg.ptr<uchar>(y);
        int x = 0;

        while (x < W) {
            uchar lbl = row[x];

            // 背景直接跳过
            if (lbl == 0) {
                ++x;
                continue;
            }

            // 同一行内，相同 label 的连续区间
            int x1 = x;
            while (x < W && row[x] == lbl) ++x;
            int x2 = x - 1;

            // label -> region
            RegionRLE& reg = regions[(int)lbl];
            if (reg.img_w == 0 && reg.img_h == 0) {
                reg.setImageSize(W, H);
            }

            reg.addRun(y, x1, x2);
        }
    }

    //合并 + 统计 area / bbox
    for (auto& kv : regions) {
        kv.second.normalize();
    }

    return regions;
}

//默认255，标签数值可以自己设置0
cv::Mat toBinary(RegionRLE reg,uint8_t fg)
{

    if (reg.img_w <= 0 || reg.img_h <= 0) {
        throw std::runtime_error("RegionRLE::toBinary(): invalid image size");
    }


    cv::Mat bin(reg.img_h, reg.img_w, CV_8UC1, cv::Scalar(0));

    if (reg.runs.empty()) return bin;

    for (const auto& r : reg.runs) {
        // 越界检查
        if (r.y < 0 || r.y >= reg.img_h) continue;
        int x1 = std::max(r.x1, 0);
        int x2 = std::min(r.x2, reg.img_w - 1);
        if (x2 < x1) continue;

        uint8_t* row = bin.ptr<uint8_t>(r.y);
        std::fill(row + x1, row + x2 + 1, fg);
    }

    return bin;
}
