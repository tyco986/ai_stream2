#include "plugins/fadedrawer/engine/pose_drawer.hpp"

#include <algorithm>
#include <cmath>

#include "constants/fadedrawer_const.hpp"

PoseDrawer::PoseDrawer(bool showPose, float poseThreshold)
    : showPose(showPose),
      poseThreshold(poseThreshold) {
}

bool PoseDrawer::keep() const {
    return showPose;
}

std::vector<float> PoseDrawer::decodeKeypoints(const float* data, unsigned int widthDim, unsigned int heightDim, float left, float top, float boxWidth, float boxHeight) const {
    const bool boxValid = data != nullptr && boxWidth > 0.0f && boxHeight > 0.0f;
    unsigned int count = 0;
    if (boxValid && widthDim == 3 && heightDim >= 1) {
        count = heightDim;
    } else if (boxValid && widthDim > 0 && heightDim == 3) {
        count = widthDim;
    }
    std::vector<float> keypoints(static_cast<std::size_t>(count) * 3);
    for (unsigned int index = 0; index < count; index++) {
        keypoints[index * 3 + 0] = left + data[index * 3 + 0] * boxWidth;
        keypoints[index * 3 + 1] = top + data[index * 3 + 1] * boxHeight;
        keypoints[index * 3 + 2] = data[index * 3 + 2];
    }
    return keypoints;
}

int PoseDrawer::clampCoord(float value, int limit) const {
    const int last = limit > 0 ? limit - 1 : 0;
    return std::clamp(static_cast<int>(std::lround(value)), 0, last);
}

void PoseDrawer::drawPose(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const std::vector<float>& keypoints, float alpha) const {
    const int pointCount = static_cast<int>(keypoints.size() / 3);
    if (showPose && pointCount >= 1) {
        const int frameWidth = frame->source_frame_width > 0 ? static_cast<int>(frame->source_frame_width) : 1;
        const int frameHeight = frame->source_frame_height > 0 ? static_cast<int>(frame->source_frame_height) : 1;
        std::vector<NvOSD_CircleParams> circles;
        circles.reserve(static_cast<std::size_t>(pointCount));
        for (int index = 0; index < pointCount; index++) {
            const float score = keypoints[static_cast<std::size_t>(index) * 3 + 2];
            const float* rgb = score < poseThreshold ? fadedrawer::COLOR_RED : fadedrawer::COLOR_ORANGE;
            const NvOSD_ColorParams color = DetFadeEngine::colorWithAlpha(rgb, alpha);
            NvOSD_CircleParams circle{};
            circle.xc = static_cast<unsigned int>(clampCoord(keypoints[static_cast<std::size_t>(index) * 3 + 0], frameWidth));
            circle.yc = static_cast<unsigned int>(clampCoord(keypoints[static_cast<std::size_t>(index) * 3 + 1], frameHeight));
            circle.radius = static_cast<unsigned int>(fadedrawer::KPT_RADIUS);
            circle.circle_color = color;
            circle.has_bg_color = 1;
            circle.bg_color = color;
            circles.push_back(circle);
        }
        std::vector<NvOSD_LineParams> lines;
        const NvOSD_ColorParams lineColor = DetFadeEngine::colorWithAlpha(fadedrawer::COLOR_ORANGE, alpha);
        for (int edge = 0; edge < fadedrawer::COCO17_EDGE_COUNT; edge++) {
            const int start = fadedrawer::COCO17_EDGES[edge][0];
            const int end = fadedrawer::COCO17_EDGES[edge][1];
            if (start < pointCount && end < pointCount) {
                NvOSD_LineParams line{};
                line.x1 = static_cast<unsigned int>(clampCoord(keypoints[static_cast<std::size_t>(start) * 3 + 0], frameWidth));
                line.y1 = static_cast<unsigned int>(clampCoord(keypoints[static_cast<std::size_t>(start) * 3 + 1], frameHeight));
                line.x2 = static_cast<unsigned int>(clampCoord(keypoints[static_cast<std::size_t>(end) * 3 + 0], frameWidth));
                line.y2 = static_cast<unsigned int>(clampCoord(keypoints[static_cast<std::size_t>(end) * 3 + 1], frameHeight));
                line.line_width = static_cast<unsigned int>(fadedrawer::SKELETON_WIDTH);
                line.line_color = lineColor;
                lines.push_back(line);
            }
        }
        std::size_t circleIndex = 0;
        std::size_t lineIndex = 0;
        while (circleIndex < circles.size() || lineIndex < lines.size()) {
            NvDsDisplayMeta* display = nvds_acquire_display_meta_from_pool(batch);
            display->num_circles = 0;
            display->num_lines = 0;
            while (circleIndex < circles.size() && display->num_circles < MAX_ELEMENTS_IN_DISPLAY_META) {
                display->circle_params[display->num_circles] = circles[circleIndex];
                display->num_circles += 1;
                circleIndex += 1;
            }
            while (lineIndex < lines.size() && display->num_lines < MAX_ELEMENTS_IN_DISPLAY_META) {
                display->line_params[display->num_lines] = lines[lineIndex];
                display->num_lines += 1;
                lineIndex += 1;
            }
            nvds_add_display_meta_to_frame(frame, display);
        }
    }
}

void PoseDrawer::clearMask(NvDsObjectMeta* object) const {
    g_free(object->mask_params.data);
    object->mask_params.data = nullptr;
    object->mask_params.size = 0;
    object->mask_params.width = 0;
    object->mask_params.height = 0;
    object->mask_params.threshold = 0.0f;
}

void PoseDrawer::drawCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const DetFadeEngine::CachedObject& cached, float alpha) const {
    const float* data = cached.mask.empty() ? nullptr : cached.mask.data();
    const std::vector<float> keypoints = decodeKeypoints(data, cached.maskWidth, cached.maskHeight, cached.left, cached.top, cached.width, cached.height);
    drawPose(batch, frame, keypoints, alpha);
}

void PoseDrawer::clearObject(NvDsObjectMeta* object) const {
    clearMask(object);
}

void PoseDrawer::drawObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) const {
    const std::vector<float> keypoints = decodeKeypoints(
        object->mask_params.data,
        object->mask_params.width,
        object->mask_params.height,
        object->rect_params.left,
        object->rect_params.top,
        object->rect_params.width,
        object->rect_params.height);
    drawPose(batch, frame, keypoints, alpha);
    clearMask(object);
}
