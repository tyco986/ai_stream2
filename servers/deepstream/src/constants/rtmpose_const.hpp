#pragma once

namespace rtmpose {

inline constexpr int INFER_WIDTH = 192;
inline constexpr int INFER_HEIGHT = 256;
inline constexpr float PADDING = 1.25f;

inline constexpr float PIXEL_NORMALIZATION_FACTOR = 0.017124753831663668f;
inline constexpr float OFFSET_R = 123.675f;
inline constexpr float OFFSET_G = 116.28f;
inline constexpr float OFFSET_B = 103.53f;

inline constexpr const char* KEY_PADDING = "padding";
inline constexpr const char* KEY_INFER_WIDTH = "infer-width";
inline constexpr const char* KEY_INFER_HEIGHT = "infer-height";
inline constexpr const char* KEY_PIXEL_NORMALIZATION_FACTOR = "pixel-normalization-factor";
inline constexpr const char* KEY_OFFSETS = "offsets";

}
