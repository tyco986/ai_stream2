#pragma once

namespace rtmposepostprocess {

inline constexpr int DEFAULT_SGIE_UNIQUE_ID = 2;
inline constexpr int MIN_KEYPOINT_ELEMENTS = 3;
inline constexpr const char* KEYPOINTS_LAYER = "keypoints";

inline constexpr const char* ELEMENT_NAME = "rtmposepostprocess";
inline constexpr const char* ELEMENT_DETAIL = "Map RTMPose crop keypoints onto object mask_params";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream RTMPose keypoints to mask_params";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream RTMPose postprocess";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define RTMPOSEPOSTPROCESS_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
