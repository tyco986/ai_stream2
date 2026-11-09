#pragma once

namespace sahipostprocesspose {

inline constexpr int COCO_KEYPOINT_COUNT = 17;
inline constexpr float COCO17_SIGMAS[COCO_KEYPOINT_COUNT] = {
    0.026f, 0.025f, 0.025f, 0.035f, 0.035f,
    0.079f, 0.079f, 0.072f, 0.072f, 0.062f,
    0.062f, 0.107f, 0.107f, 0.087f, 0.087f,
    0.089f, 0.089f};
inline constexpr float DEFAULT_SIGMA = 0.05f;
inline constexpr float AREA_EPSILON = 1e-9f;
inline constexpr const char* DEFAULT_GIE_IDS = "-1";
inline constexpr bool DEFAULT_CLASS_AGNOSTIC = false;
inline constexpr int MATCH_METRIC_IOU = 0;
inline constexpr int MATCH_METRIC_IOS = 1;
inline constexpr int DEFAULT_MATCH_METRIC = MATCH_METRIC_IOS;
inline constexpr float DEFAULT_MATCH_THRESHOLD = 0.5f;
inline constexpr bool DEFAULT_PREFER_FULL_FRAME = true;
inline constexpr int DEFAULT_MAX_DETECTIONS = -1;
inline constexpr float DEFAULT_OKS_THRESHOLD = 0.5f;
inline constexpr float DEFAULT_VIS_THRESHOLD = 0.0f;
inline constexpr int DEFAULT_NUM_KEYPOINTS = 0;

inline constexpr const char* ELEMENT_NAME = "nvsahipostprocesspose";
inline constexpr const char* ELEMENT_DETAIL = "Suppress duplicate SAHI pose instances with box overlap, then OKS";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream SAHI pose post-process";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream SAHI pose post-process";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define SAHIPOSES_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
