#pragma once

#include <cstddef>

namespace nvfadedrawer {

constexpr float kColorGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr float kColorPurple[] = {0.62f, 0.13f, 0.94f, 1.0f};
constexpr float kColorRed[] = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr float kColorYellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
constexpr float kColorOrange[] = {1.0f, 0.5f, 0.0f, 1.0f};
constexpr float kColorText[] = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr float kColorTextBg[] = {0.0f, 0.0f, 0.0f, 0.6f};

constexpr float kMinAlpha = 0.2f;
constexpr int kBoxWidth = 2;
constexpr int kFontSize = 12;
constexpr int kLabelYOffset = 14;
constexpr char kFontName[] = "Serif";
constexpr char kLabelSep = '|';

constexpr int kMaxDisplayElements = 16;

constexpr int kKptRadius = 2;
constexpr int kKptWidth = 1;
constexpr int kSkeletonWidth = 2;
constexpr int kCoco17Keypoints = 17;

constexpr unsigned long long kUntrackedObjectId = ~0ull;

constexpr int kCoco17EdgeCount = 16;
constexpr int kCoco17Edges[kCoco17EdgeCount][2] = {
    {0, 1},  {0, 2},  {1, 3},  {2, 4},  {5, 6},  {5, 7},  {7, 9},  {6, 8},
    {8, 10}, {5, 11}, {6, 12}, {11, 12}, {11, 13}, {13, 15}, {12, 14}, {14, 16},
};

}  // namespace nvfadedrawer
