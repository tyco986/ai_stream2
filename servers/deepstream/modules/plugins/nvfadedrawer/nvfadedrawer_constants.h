#pragma once

#include <cstddef>

namespace nvfadedrawer {

constexpr float kColorGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr float kColorRed[] = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr float kColorYellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
constexpr float kColorText[] = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr float kColorTextBg[] = {0.0f, 0.0f, 0.0f, 0.6f};

constexpr float kMinAlpha = 0.2f;
constexpr int kBoxWidth = 2;
constexpr int kFontSize = 12;
constexpr int kLabelYOffset = 14;
constexpr char kFontName[] = "Serif";
constexpr char kLabelSep = '|';

constexpr int kMaxDisplayElements = 16;

constexpr float kMinCachedConfidence = 0.0f;
constexpr unsigned long long kUntrackedObjectId = ~0ull;

}  // namespace nvfadedrawer
