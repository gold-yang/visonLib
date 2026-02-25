#pragma once
#include"region.h"
#include<opencv2/opencv.hpp>


/********************************     GenContourXld  生成轮廓    **********************************************************/

struct EdgePoint {
    cv::Point p1;
    cv::Point p2;
};

struct Vertex {
    cv::Point pt;
    std::vector<int> incidentEdges;

    explicit Vertex(const cv::Point& p) : pt(p) {}
};

struct GraphEdge {
    int v1;
    int v2;

    GraphEdge(int a, int b) : v1(a), v2(b) {}
};

struct EdgeGraph {
    std::vector<Vertex> vertices;
    std::vector<GraphEdge> edges;
};


static inline std::vector<RowSpan> buildRowSpansFromRuns(const std::vector<RunRLE>& runs);

static inline const RowSpan* findSpan(const std::vector<RowSpan>& spans, int y);

static std::vector<EdgePoint> regionToEdges_RLE(const RegionRLE& region0);

static EdgeGraph buildEdgeGraph(const std::vector<EdgePoint>& rawEdges, int imgWidth, int imgHeight);

static inline int otherVertex(const GraphEdge& e, int v);

std::vector<std::vector<cv::Point>> extractContours(const EdgeGraph& g);

std::vector<std::vector<cv::Point>> GenContourXld(const RegionRLE& region);

std::vector<cv::Point> findsMaxArea(const cv::Mat& src, int& maxArea, cv::Rect& rect);

struct cv_Contour2D {
    std::vector<cv::Point> contour;
    int area = 0;
    cv::Rect boundRect;
};

std::vector<cv_Contour2D> SelectShapeArea(const cv::Mat& binImage, int mins, int maxs);
