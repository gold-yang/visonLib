#pragma once

#include<opencv2/opencv.hpp>
#include "region.h"

/********************************      region 和 mask转换     **********************************************************/

std::unordered_map<int, RegionRLE> UCharLabelImageToRegions(const cv::Mat& labelImg);

cv::Mat toBinary(uint8_t fg = 255);