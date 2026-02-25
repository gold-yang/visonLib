#pragma once
#include<opencv2/opencv.hpp>

struct cv_Rectangle2D
{
    double row;
    double column;
    double phi;
    double length1;
    double length2;
    cv_Rectangle2D() {
    }
    cv_Rectangle2D(const cv::RotatedRect& cv_rect2);
    void offset(cv::Point ptOffset);
    cv_Rectangle2D offset2D(cv::Point ptOffset);
    std::vector<cv::Point> points(int xInc, int yInc);
    cv::Mat affineMat(cv::Point ptOffset = cv::Point(0, 0));
    cv::Mat invertAffineMat(cv::Point ptOffset = cv::Point(0, 0));

};


cv_Rectangle2D cv_SmallestRectangle2(const std::vector<cv::Point>& contour);