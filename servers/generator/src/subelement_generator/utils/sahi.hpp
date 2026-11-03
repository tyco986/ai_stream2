#pragma once

#include <map>
#include <string>

#include <opencv2/core.hpp>

struct SahiBox {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
};

struct SahiInfo {
    int width = 0;
    int height = 0;
    int row = 0;
    int column = 0;
    int num = 0;
    std::map<std::string, SahiBox> box;
};

class Sahi {
public:
    SahiInfo boxes(
        int imageWidth,
        int imageHeight,
        int sliceWidth,
        int sliceHeight,
        double overlapWidthRatio,
        double overlapHeightRatio,
        bool enableFullFrame) const;

    cv::Mat preview(const SahiInfo& info) const;

private:
    cv::Scalar color(const std::string& label, int rowCount) const;
};
