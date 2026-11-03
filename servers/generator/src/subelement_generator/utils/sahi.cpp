#include "subelement_generator/utils/sahi.hpp"

#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "constants/sahi_const.hpp"

SahiInfo Sahi::boxes(
    int imageWidth,
    int imageHeight,
    int sliceWidth,
    int sliceHeight,
    double overlapWidthRatio,
    double overlapHeightRatio,
    bool enableFullFrame) const {
    const int widthStep = std::max(1, sliceWidth - static_cast<int>(sliceWidth * overlapWidthRatio));
    const int heightStep = std::max(1, sliceHeight - static_cast<int>(sliceHeight * overlapHeightRatio));
    std::map<std::string, SahiBox> box;
    int row = 0;
    int gridColumns = 0;
    int y = 0;
    while (y < imageHeight) {
        const int yEnd = std::min(y + sliceHeight, imageHeight);
        int yStart = y;
        if (yEnd == imageHeight && yStart > 0 && (yEnd - yStart) < sliceHeight) {
            yStart = std::max(0, imageHeight - sliceHeight);
        }
        int col = 0;
        int x = 0;
        while (x < imageWidth) {
            const int xEnd = std::min(x + sliceWidth, imageWidth);
            int xStart = x;
            if (xEnd == imageWidth && xStart > 0 && (xEnd - xStart) < sliceWidth) {
                xStart = std::max(0, imageWidth - sliceWidth);
            }
            box[std::to_string(row) + "-" + std::to_string(col)] = SahiBox{xStart, yStart, xEnd, yEnd};
            col += 1;
            if (xEnd >= imageWidth) {
                break;
            }
            x += widthStep;
        }
        gridColumns = std::max(gridColumns, col);
        row += 1;
        if (yEnd >= imageHeight) {
            break;
        }
        y += heightStep;
    }
    if (enableFullFrame) {
        box["full"] = SahiBox{0, 0, imageWidth, imageHeight};
    }
    SahiInfo info;
    info.width = imageWidth;
    info.height = imageHeight;
    info.row = row;
    info.column = gridColumns;
    info.num = static_cast<int>(box.size());
    info.box = std::move(box);
    return info;
}

cv::Scalar Sahi::color(const std::string& label, int rowCount) const {
    cv::Scalar ink(255, 255, 255);
    if (label != "full") {
        const int row = std::stoi(label.substr(0, label.find('-')));
        const double hue = static_cast<double>(row) / std::max(rowCount, 1);
        cv::Mat hsv(1, 1, CV_8UC3, cv::Scalar(hue * 180.0, sahi::SATURATION * 255.0, 255));
        cv::Mat bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
        const cv::Vec3b pixel = bgr.at<cv::Vec3b>(0, 0);
        ink = cv::Scalar(pixel[0], pixel[1], pixel[2]);
    }
    return ink;
}

cv::Mat Sahi::preview(const SahiInfo& info) const {
    cv::Mat image(info.height, info.width, CV_8UC3, cv::Scalar(0, 0, 0));
    for (const auto& item : info.box) {
        const cv::Scalar ink = color(item.first, info.row);
        const SahiBox& box = item.second;
        cv::rectangle(
            image,
            cv::Point(box.x1, box.y1),
            cv::Point(box.x2 - 1, box.y2 - 1),
            ink,
            sahi::OUTLINE_WIDTH);
        const int textY = box.y1 + sahi::TEXT_CLEARANCE < info.height
            ? box.y1 + sahi::TEXT_PAD
            : std::max(0, box.y1 - sahi::TEXT_LIFT);
        cv::putText(
            image,
            item.first,
            cv::Point(box.x1 + sahi::TEXT_PAD, textY + sahi::TEXT_CLEARANCE),
            sahi::FONT_FACE,
            sahi::FONT_SCALE,
            ink,
            1,
            cv::LINE_AA);
    }
    return image;
}
