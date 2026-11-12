#pragma once

namespace pipeline_runner {

inline constexpr const char* PARAMS_FILE = "params.yml";
inline constexpr const char* PIPELINE_FILE = "pipeline.yml";
inline constexpr const char* PIPELINE_NAME_KEY = "pipeline_name";
inline constexpr const char* SOURCE_PREFIX = "nvurisrcbin";
inline constexpr const char* MUX_NAME = "nvstreammux";
inline constexpr const char* DEMUX_NAME = "nvstreamdemux";
inline constexpr const char* DEMUX_QUEUE_PREFIX = "queue_demux";
inline constexpr const char* MUX_SINK_PAD = "sink_%u";
inline constexpr const char* DEMUX_SRC_PAD = "src_%u";
inline constexpr const char* FILE_SINK = "filesink";
inline constexpr const char* LOCATION_KEY = "location";
inline constexpr const char* LOG_DIR = "/root/logs/deepstream";
inline constexpr const char* LOG_FILE = "pipeline.log";

}
