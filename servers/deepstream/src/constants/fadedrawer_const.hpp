#pragma once

#include <cstdint>

#include "nvdsmeta.h"

namespace fadedrawer {

inline constexpr float COLOR_GREEN[4] = {0.0f, 1.0f, 0.0f, 1.0f};
inline constexpr float COLOR_RED[4] = {1.0f, 0.0f, 0.0f, 1.0f};
inline constexpr float COLOR_YELLOW[4] = {1.0f, 1.0f, 0.0f, 1.0f};
inline constexpr float COLOR_TEXT[4] = {1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr float COLOR_TEXT_BG[4] = {0.0f, 0.0f, 0.0f, 0.6f};
inline constexpr float COLOR_ORANGE[4] = {1.0f, 0.5f, 0.0f, 1.0f};
inline constexpr float COLOR_PURPLE[4] = {0.62f, 0.13f, 0.94f, 1.0f};
inline constexpr float MIN_CACHED_CONFIDENCE = 0.0f;
inline constexpr std::uint64_t UNTRACKED_OBJECT_ID = ~0ull;

inline constexpr int KPT_RADIUS = 2;
inline constexpr int SKELETON_WIDTH = 2;
inline constexpr int COCO17_EDGE_COUNT = 16;
inline constexpr int COCO17_EDGES[COCO17_EDGE_COUNT][2] = {
    {0, 1}, {0, 2}, {1, 3}, {2, 4}, {5, 6}, {5, 7}, {7, 9}, {6, 8},
    {8, 10}, {5, 11}, {6, 12}, {11, 12}, {11, 13}, {13, 15}, {12, 14}, {14, 16},
};

inline constexpr float MIN_ALPHA = 0.2f;
inline constexpr int BOX_WIDTH = 2;
inline constexpr int FONT_SIZE = 12;
inline constexpr int LABEL_Y_OFFSET = 14;
inline constexpr char FONT_NAME[] = "Serif";
inline constexpr char LABEL_SEP = '|';
inline constexpr int LABEL_LINE_SIZE = 160;

inline constexpr NvDsMetaType PRESENCE_EVENT_USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5045);
inline constexpr unsigned PRESENCE_EVENT_CODE_LEN = 8;

}
