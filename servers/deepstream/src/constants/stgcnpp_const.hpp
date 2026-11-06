#pragma once

#include "nvdsmeta.h"

namespace stgcnpp {

inline constexpr int MIN_CLIP_LEN = 3;
inline constexpr int MAX_CLIP_LEN = 300;
inline constexpr int CLIP_LEN = 100;
inline constexpr int NUM_JOINTS = 17;
inline constexpr int NUM_PERSON = 2;
inline constexpr int CHANNELS = 3;
inline constexpr int PATIENCE = 0;

inline constexpr const char* KEY_CLIP_LEN = "frames-sequence-length";
inline constexpr const char* KEY_NUM_JOINTS = "num-joints";
inline constexpr const char* KEY_NUM_PERSON = "num-person";
inline constexpr const char* KEY_PATIENCE = "patience";

inline constexpr NvDsMetaType READY_USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5347);

}
