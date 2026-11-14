#pragma once

namespace pipeline {

inline constexpr const char* DEEPSTREAM_OUTPUT_ROOT = "/root/outputs/deepstream";
inline constexpr const char* DEFAULT_CAPS = "video/x-raw(memory:NVMM), format=I420";
inline constexpr const char* H264_CAPS = "video/x-raw(memory:NVMM), format=NV12";
inline constexpr const char* POSE_MODE_COCO17 = "coco17";
inline constexpr const char* PRESENCE_MODE_FOLD = "fold";
inline constexpr const char* LABEL_TASK_DET = "det";
inline constexpr const char* CAPTURE_CODES_DEFAULT = "12";
inline constexpr const char* TARGET_UNIQUE_IDS_DEFAULT = "1";
inline constexpr const char* GIE_IDS_DEFAULT = "1";

inline constexpr const char* URI_RTSP = "rtsp://";
inline constexpr const char* RTSP_RECONNECT_INTERVAL_KEY = "rtsp-reconnect-interval";
inline constexpr const char* RTSP_RECONNECT_ATTEMPTS_KEY = "rtsp-reconnect-attempts";

inline constexpr int RTP_PROTOCOL_TCP = 4;
inline constexpr int RTSP_RECONNECT_INTERVAL = 10;
inline constexpr int RTSP_RECONNECT_ATTEMPTS = 18;
inline constexpr int BATCHED_PUSH_TIMEOUT_US = 40000;
inline constexpr int SLICE_SIZE = 640;
inline constexpr double OVERLAP_RATIO = 0.2;
inline constexpr int PREPROCESS_UNIQUE_ID = 15;
inline constexpr int MATCH_METRIC_IOS = 1;
inline constexpr double MATCH_THRESHOLD = 0.5;
inline constexpr double PRESENCE_THRESHOLD = 0.5;
inline constexpr int POSE_SGIE_UNIQUE_ID = 2;
inline constexpr int CLASSIFIER_UNIQUE_ID = 4;
inline constexpr double OKS_THRESHOLD = 0.5;
inline constexpr int QUEUE_LEAKY_DOWNSTREAM = 2;
inline constexpr int QUEUE_MAX_BUFFERS = 4;
inline constexpr int PRESENCE_LENGTH = 10;
inline constexpr int PAYLOAD_DEEPSTREAM_MINIMAL = 1;
inline constexpr int FRAME_INTERVAL = 1;
inline constexpr int JPEG_QUALITY = 85;
inline constexpr int H264_BITRATE = 4000000;
inline constexpr int H264_IFRAME_INTERVAL = 25;
inline constexpr int H264_PRESET_ID = 1;

}
