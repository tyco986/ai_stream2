#include "plugins/nvdspreprocess/rtmposepreprocess/engine/rect_expand.hpp"

#include <algorithm>

RectExpand::RectExpand(int inferWidth, int inferHeight, float padding) {
    this->inferWidth = inferWidth;
    this->inferHeight = inferHeight;
    this->padding = padding;
}

CropGeom RectExpand::compute(float left, float top, float width, float height, int frameWidth, int frameHeight) const {
    float expandedLeft = 0.0f;
    float expandedTop = 0.0f;
    float expandedWidth = 0.0f;
    float expandedHeight = 0.0f;
    expand(left, top, width, height, frameWidth, frameHeight, &expandedLeft, &expandedTop, &expandedWidth, &expandedHeight);
    CropGeom geom;
    alignSource(expandedLeft, expandedTop, expandedWidth, expandedHeight, &geom.srcLeft, &geom.srcTop, &geom.srcWidth, &geom.srcHeight);
    if (geom.srcLeft + geom.srcWidth > frameWidth) {
        geom.srcWidth = std::max(2, alignDown2(frameWidth - geom.srcLeft));
    }
    if (geom.srcTop + geom.srcHeight > frameHeight) {
        geom.srcHeight = std::max(2, alignDown2(frameHeight - geom.srcTop));
    }
    letterbox(geom.srcWidth, geom.srcHeight, &geom.destWidth, &geom.destHeight, &geom.offsetLeft, &geom.offsetTop);
    return geom;
}

void RectExpand::expand(
    float left,
    float top,
    float width,
    float height,
    int frameWidth,
    int frameHeight,
    float* outLeft,
    float* outTop,
    float* outWidth,
    float* outHeight) const {
    float aspect = inferWidth / static_cast<float>(inferHeight);
    float centerX = left + width * 0.5f;
    float centerY = top + height * 0.5f;
    float scaleW = width * padding;
    float scaleH = height * padding;
    if (scaleW > aspect * scaleH) {
        scaleH = scaleW / aspect;
    } else {
        scaleW = scaleH * aspect;
    }
    float expandedLeft = std::max(0.0f, centerX - scaleW * 0.5f);
    float expandedTop = std::max(0.0f, centerY - scaleH * 0.5f);
    float expandedRight = std::min(static_cast<float>(frameWidth), centerX + scaleW * 0.5f);
    float expandedBottom = std::min(static_cast<float>(frameHeight), centerY + scaleH * 0.5f);
    *outLeft = expandedLeft;
    *outTop = expandedTop;
    *outWidth = std::max(2.0f, expandedRight - expandedLeft);
    *outHeight = std::max(2.0f, expandedBottom - expandedTop);
}

void RectExpand::alignSource(
    float left,
    float top,
    float width,
    float height,
    int* srcLeft,
    int* srcTop,
    int* srcWidth,
    int* srcHeight) const {
    int alignedWidth = alignDown2(static_cast<int>(width));
    int alignedHeight = alignDown2(static_cast<int>(height));
    if (alignedWidth < 2) {
        alignedWidth = 2;
    }
    if (alignedHeight < 2) {
        alignedHeight = 2;
    }
    *srcLeft = alignUp2(static_cast<int>(left));
    *srcTop = alignUp2(static_cast<int>(top));
    *srcWidth = alignedWidth;
    *srcHeight = alignedHeight;
}

void RectExpand::letterbox(int srcWidth, int srcHeight, int* destWidth, int* destHeight, int* offsetLeft, int* offsetTop) const {
    float fitHeight = inferWidth * srcHeight / static_cast<float>(srcWidth);
    int boxWidth = inferWidth;
    int boxHeight = static_cast<int>(fitHeight);
    if (fitHeight > inferHeight) {
        boxWidth = static_cast<int>(inferHeight * srcWidth / static_cast<float>(srcHeight));
        boxHeight = inferHeight;
    }
    *destWidth = boxWidth;
    *destHeight = boxHeight;
    *offsetLeft = (inferWidth - boxWidth) / 2;
    *offsetTop = (inferHeight - boxHeight) / 2;
}

int RectExpand::alignUp2(int value) const {
    return (value + 1) & ~1;
}

int RectExpand::alignDown2(int value) const {
    return value & ~1;
}
