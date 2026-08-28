#pragma once

void rgba_to_nchw(
    const unsigned char *src,
    int src_pitch,
    int width,
    int height,
    float *dst,
    float scale,
    float offset_r,
    float offset_g,
    float offset_b);
