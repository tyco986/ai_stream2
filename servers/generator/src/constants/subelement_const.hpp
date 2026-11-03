#pragma once

namespace subelement {

inline constexpr const char* KAFKA_PROTO_LIB = "/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so";
inline constexpr const char* KAFKA_CONN_STR = "ai_stream2_kafka;9092";
inline constexpr const char* KAFKA_CONFIG =
    "[message-broker]\n"
    "#producer-proto-cfg = \"queue.buffering.max.messages=200000;message.send.max.retries=3\"\n";

inline constexpr const char* TRACKER_LL_LIB = "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so";

inline constexpr const char* SAHI_CUSTOM_LIB_PATH = "/opt/nvidia/deepstream/deepstream/lib/gst-plugins/libcustom2d_preprocess.so";
inline constexpr const char* SAHI_NETWORK_INPUT_SHAPE = "16;3;640;640";
inline constexpr const char* SAHI_TENSOR_NAME = "input";

inline constexpr const char* RTMPOSE_CUSTOM_LIB_PATH = "/opt/ai_stream2/servers/deepstream/libs/libnvds_rtmpose_preprocess.so";
inline constexpr int RTMPOSE_UNIQUE_ID = 5;
inline constexpr int RTMPOSE_TARGET_UNIQUE_IDS = 2;
inline constexpr int RTMPOSE_OPERATE_ON_GIE_ID = 1;
inline constexpr const char* RTMPOSE_TENSOR_NAME = "input";
inline constexpr const char* RTMPOSE_PADDING = "1.25";
inline constexpr const char* RTMPOSE_PIXEL_NORMALIZATION_FACTOR = "0.017124753831663668";
inline constexpr const char* RTMPOSE_OFFSETS = "123.675;116.28;103.53";
inline constexpr const char* RTMPOSE_OPERATE_ON_CLASS_IDS = "0";

inline constexpr const char* STGCNPP_CUSTOM_LIB_PATH = "/opt/ai_stream2/servers/deepstream/libs/libnvds_stgcnpp_preprocess.so";
inline constexpr int STGCNPP_UNIQUE_ID = 3;
inline constexpr int STGCNPP_TARGET_UNIQUE_IDS = 4;

inline constexpr int NVDSANALYTICS_DISPLAY_FONT_SIZE = 12;
inline constexpr const char* NVDSANALYTICS_RULE_NAMES[] = {
    "roi_filtering",
    "overcrowding",
    "line_crossing",
    "direction_detection",
};

}
