#include "plugins/sahipostprocesspose/engine/sahi_pose_nms.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <tuple>
#include <unordered_map>

#include "constants/sahipostprocesspose_const.hpp"
#include "plugins/sahipostprocesspose/engine/spatial_grid.hpp"

namespace {

bool betterPose(const std::vector<PoseDetection>& detections, unsigned int left, unsigned int right) {
    const PoseDetection& first = detections[left];
    const PoseDetection& second = detections[right];
    const auto firstBox = std::tie(first.left, first.top, first.right, first.bottom);
    const auto secondBox = std::tie(second.left, second.top, second.right, second.bottom);
    const bool higherScore = first.score > second.score;
    const bool sameScoreSmallerBox = first.score == second.score && firstBox < secondBox;
    return higherScore || sameScoreSmallerBox;
}

struct ScoreOrder {
    const std::vector<PoseDetection>* detections;
    bool operator()(unsigned int left, unsigned int right) const {
        return betterPose(*detections, left, right);
    }
};

}

SahiPoseNms::SahiPoseNms(const char* gieIdsText, bool classAgnostic, int matchMetric, float matchThreshold, bool preferFullFrame, int maxDetections, float oksThreshold, float visThreshold, int numKeypoints)
    : gieIdsText(gieIdsText != nullptr ? gieIdsText : sahipostprocesspose::DEFAULT_GIE_IDS),
      classAgnostic(classAgnostic),
      matchMetric(matchMetric),
      matchThreshold(matchThreshold),
      preferFullFrame(preferFullFrame),
      maxDetections(maxDetections),
      oksThreshold(oksThreshold),
      visThreshold(visThreshold),
      numKeypoints(numKeypoints),
      filterAll(true) {
    parseGieIds();
}

void SahiPoseNms::parseGieIds() {
    gieIds.clear();
    filterAll = false;
    std::string pending;
    for (char character : gieIdsText + ";") {
        if (character == ';') {
            const std::string token = pending;
            pending.clear();
            const char* begin = token.c_str();
            while (*begin == ' ' || *begin == '\t') {
                begin++;
            }
            if (*begin != '\0' && std::atoi(begin) < 0) {
                gieIds.clear();
                filterAll = true;
                pending.clear();
                break;
            } else if (*begin != '\0') {
                gieIds.insert(std::atoi(begin));
            }
        } else {
            pending.push_back(character);
        }
    }
    filterAll = filterAll || gieIds.empty();
}

bool SahiPoseNms::accepts(const NvDsObjectMeta* objectMeta) const {
    const bool listed = gieIds.find(objectMeta->unique_component_id) != gieIds.end();
    return filterAll || listed;
}

std::vector<float> SahiPoseNms::keypointsOf(const NvDsObjectMeta* objectMeta) const {
    const float boxWidth = objectMeta->rect_params.width;
    const float boxHeight = objectMeta->rect_params.height;
    const unsigned int available = objectMeta->mask_params.height;
    const unsigned int count = numKeypoints > 0 ? static_cast<unsigned int>(numKeypoints) : available;
    const bool usable = objectMeta->mask_params.data != nullptr && objectMeta->mask_params.width == 3 && available >= count && count >= 1 && boxWidth > 0.0f && boxHeight > 0.0f;
    std::vector<float> keypoints;
    if (usable) {
        keypoints.resize(count * 3);
        for (unsigned int index = 0; index < count; index++) {
            keypoints[index * 3 + 0] = objectMeta->rect_params.left + objectMeta->mask_params.data[index * 3 + 0] * boxWidth;
            keypoints[index * 3 + 1] = objectMeta->rect_params.top + objectMeta->mask_params.data[index * 3 + 1] * boxHeight;
            keypoints[index * 3 + 2] = objectMeta->mask_params.data[index * 3 + 2];
        }
    }
    return keypoints;
}

float SahiPoseNms::overlap(const PoseDetection& left, const PoseDetection& right) const {
    const float interLeft = std::max(left.left, right.left);
    const float interTop = std::max(left.top, right.top);
    const float interRight = std::min(left.right, right.right);
    const float interBottom = std::min(left.bottom, right.bottom);
    const float width = interRight - interLeft;
    const float height = interBottom - interTop;
    float value = 0.0f;
    if (width > 0.0f && height > 0.0f) {
        const float inter = width * height;
        const float unionArea = left.area + right.area - inter;
        const float smaller = std::min(left.area, right.area);
        const float denominator = matchMetric == sahipostprocesspose::MATCH_METRIC_IOU ? unionArea : smaller;
        value = denominator > 0.0f ? inter / denominator : 0.0f;
    }
    return value;
}

float SahiPoseNms::keypointVar(unsigned int index, unsigned int keypointCount) const {
    const bool coco = keypointCount == static_cast<unsigned int>(sahipostprocesspose::COCO_KEYPOINT_COUNT) && index < static_cast<unsigned int>(sahipostprocesspose::COCO_KEYPOINT_COUNT);
    const float sigma = coco ? sahipostprocesspose::COCO17_SIGMAS[index] : sahipostprocesspose::DEFAULT_SIGMA;
    const float doubled = sigma * 2.0f;
    return doubled * doubled;
}

float SahiPoseNms::oks(const PoseDetection& left, const PoseDetection& right) const {
    float value = 0.0f;
    const bool comparable = left.keypoints.size() == right.keypoints.size() && !left.keypoints.empty();
    if (comparable) {
        const unsigned int count = left.keypoints.size() / 3;
        const float scale = (left.area + right.area) * 0.5f + sahipostprocesspose::AREA_EPSILON;
        float sum = 0.0f;
        unsigned int visible = 0;
        for (unsigned int index = 0; index < count; index++) {
            const float leftScore = left.keypoints[index * 3 + 2];
            const float rightScore = right.keypoints[index * 3 + 2];
            if (leftScore > visThreshold && rightScore > visThreshold) {
                const float dx = left.keypoints[index * 3 + 0] - right.keypoints[index * 3 + 0];
                const float dy = left.keypoints[index * 3 + 1] - right.keypoints[index * 3 + 1];
                const float energy = (dx * dx + dy * dy) / keypointVar(index, count) / scale / 2.0f;
                sum += std::exp(-energy);
                visible++;
            }
        }
        value = sum / static_cast<float>(std::max(visible, 1u));
    }
    return value;
}

bool SahiPoseNms::anchored(const PoseDetection& detection) const {
    const bool kept = preferFullFrame && detection.fromFullFrame;
    return kept;
}

void SahiPoseNms::suppressAnchors(const std::vector<PoseDetection>& detections, std::vector<char>& suppressed) const {
    std::vector<unsigned int> anchors;
    for (unsigned int index = 0; index < detections.size(); index++) {
        if (detections[index].fromFullFrame) {
            anchors.push_back(index);
        }
    }
    for (unsigned int index = 0; index < detections.size() && !anchors.empty(); index++) {
        if (!detections[index].fromFullFrame && !suppressed[index]) {
            for (unsigned int anchor : anchors) {
                const bool sameClass = detections[index].classId == detections[anchor].classId;
                if ((classAgnostic || sameClass) && overlap(detections[anchor], detections[index]) >= matchThreshold) {
                    suppressed[index] = 1;
                    break;
                }
            }
        }
    }
}

void SahiPoseNms::suppressByOverlap(const SpatialGrid& grid, const std::vector<PoseDetection>& detections, std::vector<char>& suppressed, const std::vector<unsigned int>& indexes) const {
    std::vector<unsigned int> candidates;
    for (unsigned int index : indexes) {
        if (!suppressed[index]) {
            grid.query({detections[index].left, detections[index].top, detections[index].right, detections[index].bottom}, candidates);
            for (unsigned int candidate : candidates) {
                const bool sameClass = detections[index].classId == detections[candidate].classId;
                const bool comparable = candidate != index && !suppressed[candidate] && !anchored(detections[candidate]) && (classAgnostic || sameClass) && !betterPose(detections, candidate, index);
                if (comparable && overlap(detections[index], detections[candidate]) >= matchThreshold) {
                    suppressed[candidate] = 1;
                }
            }
        }
    }
}

void SahiPoseNms::suppressByOks(const SpatialGrid& grid, const std::vector<PoseDetection>& detections, std::vector<char>& suppressed, const std::vector<unsigned int>& indexes) const {
    std::vector<unsigned int> candidates;
    for (unsigned int index : indexes) {
        if (!suppressed[index]) {
            grid.query({detections[index].left, detections[index].top, detections[index].right, detections[index].bottom}, candidates);
            for (unsigned int candidate : candidates) {
                const bool sameClass = detections[index].classId == detections[candidate].classId;
                const bool comparable = candidate != index && !suppressed[candidate] && !anchored(detections[candidate]) && (classAgnostic || sameClass) && !betterPose(detections, candidate, index);
                if (comparable && oks(detections[index], detections[candidate]) > oksThreshold) {
                    suppressed[candidate] = 1;
                }
            }
        }
    }
}

void SahiPoseNms::capDetections(const std::vector<PoseDetection>& detections, std::vector<char>& suppressed) const {
    std::vector<unsigned int> survivors;
    for (unsigned int index = 0; index < detections.size(); index++) {
        if (!suppressed[index]) {
            survivors.push_back(index);
        }
    }
    if (maxDetections >= 0 && static_cast<int>(survivors.size()) > maxDetections) {
        const ScoreOrder scoreOrder{&detections};
        std::sort(survivors.begin(), survivors.end(), scoreOrder);
        for (unsigned int index = static_cast<unsigned int>(maxDetections); index < survivors.size(); index++) {
            suppressed[survivors[index]] = 1;
        }
    }
}

void SahiPoseNms::suppress(NvDsFrameMeta* frame) {
    std::vector<PoseDetection> detections;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* objectMeta = static_cast<NvDsObjectMeta*>(item->data);
        std::vector<float> keypoints = objectMeta != nullptr && accepts(objectMeta) ? keypointsOf(objectMeta) : std::vector<float>();
        if (!keypoints.empty()) {
            PoseDetection detection;
            detection.left = objectMeta->rect_params.left;
            detection.top = objectMeta->rect_params.top;
            detection.right = detection.left + objectMeta->rect_params.width;
            detection.bottom = detection.top + objectMeta->rect_params.height;
            detection.score = objectMeta->confidence;
            detection.classId = objectMeta->class_id;
            detection.area = objectMeta->rect_params.width * objectMeta->rect_params.height;
            detection.objectMeta = objectMeta;
            detection.fromFullFrame = objectMeta->misc_obj_info[0] == 1;
            detection.keypoints = std::move(keypoints);
            detections.push_back(std::move(detection));
        }
    }
    if (detections.size() > 1) {
        std::vector<unsigned int> order(detections.size());
        std::iota(order.begin(), order.end(), 0);
        const ScoreOrder scoreOrder{&detections};
        std::sort(order.begin(), order.end(), scoreOrder);
        float frameWidth = frame->source_frame_width > 0 ? static_cast<float>(frame->source_frame_width) : 0.0f;
        float frameHeight = frame->source_frame_height > 0 ? static_cast<float>(frame->source_frame_height) : 0.0f;
        if (frameWidth <= 0.0f || frameHeight <= 0.0f) {
            frameWidth = 0.0f;
            frameHeight = 0.0f;
            for (const PoseDetection& detection : detections) {
                frameWidth = std::max(frameWidth, detection.right);
                frameHeight = std::max(frameHeight, detection.bottom);
            }
        }
        std::vector<GridRect> rects;
        rects.reserve(detections.size());
        for (const PoseDetection& detection : detections) {
            rects.push_back({detection.left, detection.top, detection.right, detection.bottom});
        }
        SpatialGrid grid;
        grid.build(rects, frameWidth, frameHeight);
        std::vector<char> suppressed(detections.size(), 0);
        if (preferFullFrame) {
            suppressAnchors(detections, suppressed);
        }
        std::vector<unsigned int> participants;
        for (unsigned int index : order) {
            if (!anchored(detections[index])) {
                participants.push_back(index);
            }
        }
        if (classAgnostic) {
            suppressByOverlap(grid, detections, suppressed, participants);
            suppressByOks(grid, detections, suppressed, participants);
        } else {
            std::unordered_map<int, std::vector<unsigned int>> byClass;
            for (unsigned int index : participants) {
                byClass[detections[index].classId].push_back(index);
            }
            for (const auto& entry : byClass) {
                suppressByOverlap(grid, detections, suppressed, entry.second);
                suppressByOks(grid, detections, suppressed, entry.second);
            }
        }
        capDetections(detections, suppressed);
        for (unsigned int index = 0; index < detections.size(); index++) {
            if (suppressed[index]) {
                nvds_remove_obj_meta_from_frame(frame, detections[index].objectMeta);
            }
        }
    }
}
