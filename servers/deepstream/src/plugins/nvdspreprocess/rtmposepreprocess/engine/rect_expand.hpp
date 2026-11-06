#pragma once

struct CropGeom {
    int srcLeft = 0;
    int srcTop = 0;
    int srcWidth = 2;
    int srcHeight = 2;
    int destWidth = 0;
    int destHeight = 0;
    int offsetLeft = 0;
    int offsetTop = 0;
};

class RectExpand {
public:
    RectExpand(int inferWidth, int inferHeight, float padding);

    CropGeom compute(float left, float top, float width, float height, int frameWidth, int frameHeight) const;

    int inferWidth;
    int inferHeight;
    float padding;

private:
    void expand(
        float left,
        float top,
        float width,
        float height,
        int frameWidth,
        int frameHeight,
        float* outLeft,
        float* outTop,
        float* outWidth,
        float* outHeight) const;
    void alignSource(
        float left,
        float top,
        float width,
        float height,
        int* srcLeft,
        int* srcTop,
        int* srcWidth,
        int* srcHeight) const;
    void letterbox(int srcWidth, int srcHeight, int* destWidth, int* destHeight, int* offsetLeft, int* offsetTop) const;
    int alignUp2(int value) const;
    int alignDown2(int value) const;
};
