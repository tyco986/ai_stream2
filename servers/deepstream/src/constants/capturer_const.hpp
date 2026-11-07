#pragma once

namespace capturer {

inline constexpr const char* OUTPUT_DIR = "/root/output";
inline constexpr const char* DEFAULT_CAPTURE_CODES = "12";
inline constexpr const char* DEFAULT_LABEL_TASK = "det";
inline constexpr const char* LABEL_TASK_SEG = "seg";
inline constexpr int JPEG_QUALITY = 90;
inline constexpr char FALLBACK_CODE = '1';

inline constexpr const char* ELEMENT_RAW = "rawcapturer";
inline constexpr const char* ELEMENT_VIS = "viscapturer";
inline constexpr const char* ELEMENT_RAW_DETAIL = "Dump raw RGB frames as PNG when presence event codes match";
inline constexpr const char* ELEMENT_VIS_DETAIL = "Dump visualized RGB frames as JPEG and bbox labels when presence event codes match";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream raw and vis capture elements";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream capturer";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define CAPTURER_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ RGB, RGBA }")
