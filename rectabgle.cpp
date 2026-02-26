#include"rectangle.h"


/********************************     Rectangle2D 旋转矩形操作     **********************************************************/

cv_Rectangle2D::cv_Rectangle2D(const cv::RotatedRect& cv_rect2)
{
    this->column = cv_rect2.center.x;
    this->row = cv_rect2.center.y;

    double w = cv_rect2.size.width;
    double h = cv_rect2.size.height;

    // OpenCV angle 是“width 边”与 x 轴的角度
    double angle = cv_rect2.angle * CV_PI / 180.0;

    if (w >= h) {
        // width 就是长边
        this->length1 = w * 0.5;
        this->length2 = h * 0.5;
        this->phi = angle;
    }
    else {
        // height 是长边，方向需要旋转 90°
        this->length1 = h * 0.5;
        this->length2 = w * 0.5;
        this->phi = angle + CV_PI / 2.0;
    }

    if (this->phi < -CV_PI / 2)
        this->phi += CV_PI;
    else if (this->phi >= CV_PI / 2)
        this->phi -= CV_PI;

}

std::vector<cv::Point> cv_Rectangle2D::points(int xInc, int yInc)
{
    cv::RotatedRect rot_rect(cv::Point(this->column, this->row), cv::Size((this->length1 * 2 + xInc), (this->length2 * 2 + yInc)), this->phi * 180 / CV_PI);
    cv::Point2f pts[4];
    rot_rect.points(pts);
    std::vector<cv::Point> pts_int;
    for (int i = 0; i < 4; i++)
    {
        pts_int.push_back(cv::Point(cvRound(pts[i].x), cvRound(pts[i].y)));
    }
    return pts_int;
}



void cv_Rectangle2D::offset(cv::Point ptOffset)
{
    this->column += ptOffset.x;
    this->row += ptOffset.y;
}

////偏移
cv_Rectangle2D cv_Rectangle2D::offset2D(cv::Point ptOffset)
{
    cv_Rectangle2D t_rect2 = *this;
    t_rect2.offset(ptOffset);
    return t_rect2;
}


//旋转正逆变换
cv::Mat cv_Rectangle2D::affineMat(cv::Point ptOffset)
{
    auto mat2d = cv::getRotationMatrix2D(cv::Point(this->column, this->row), this->phi * 180 / CV_PI, 1.0);
    return mat2d;
}

cv::Mat cv_Rectangle2D::invertAffineMat(cv::Point ptOffset)
{
    auto mat2d = cv::getRotationMatrix2D(cv::Point(this->column, this->row), -this->phi * 180 / CV_PI, 1.0);
    return mat2d;
}

cv_Rectangle2D cv_SmallestRectangle2(const std::vector<cv::Point>& contour)
{
    if (contour.size() < 3)
        return cv_Rectangle2D();

    cv::RotatedRect r = cv::minAreaRect(contour);
    return cv_Rectangle2D(r);
}
