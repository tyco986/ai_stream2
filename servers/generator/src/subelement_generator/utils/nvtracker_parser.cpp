#include "subelement_generator/utils/nvtracker_parser.hpp"

#include <algorithm>
#include <stdexcept>

#include "constants/nvtracker_parser_const.hpp"

NvtrackerParser::NvtrackerParser(int interval)
    : interval(interval),
      maxShadowTrackingAge(interval + nvtracker_parser::SHADOW_AGE_OFFSET),
      earlyTerminationAge(interval + nvtracker_parser::EARLY_AGE_OFFSET),
      probationAge(nvtracker_parser::PROBATION_AGE) {
}

int NvtrackerParser::alignTrackerDimension(int value) const {
    const int aligned = (value / nvtracker_parser::DIMENSION_ALIGN) * nvtracker_parser::DIMENSION_ALIGN;
    return aligned;
}

std::vector<int> NvtrackerParser::classIds(const YAML::Node& classId) const {
    std::vector<int> ids;
    if (classId.IsSequence()) {
        for (const auto& item : classId) {
            ids.push_back(item.as<int>());
        }
    } else {
        ids.push_back(classId.as<int>());
    }
    return ids;
}

bool NvtrackerParser::validate(const YAML::Node& tracker, const std::set<int>& pgieClassIds) const {
    bool result = false;
    if (tracker && !tracker.IsNull()) {
        if (!tracker.IsMap() || tracker.size() != 1 || !tracker["class_id"]) {
            throw std::invalid_argument("tracker dict must only contain class_id");
        }
        const std::vector<int> ids = classIds(tracker["class_id"]);
        if (ids.empty()) {
            throw std::invalid_argument("tracker class_id cannot be empty");
        }
        if (ids.size() != 1 || ids[0] != -1) {
            std::vector<int> invalid;
            for (const int classId : ids) {
                if (!pgieClassIds.contains(classId)) {
                    invalid.push_back(classId);
                }
            }
            std::sort(invalid.begin(), invalid.end());
            if (!invalid.empty()) {
                std::string text;
                for (const int classId : invalid) {
                    if (!text.empty()) {
                        text += ",";
                    }
                    text += std::to_string(classId);
                }
                throw std::invalid_argument("tracker class_id not in pgie classes: " + text);
            }
        }
        result = true;
    }
    return result;
}

std::string NvtrackerParser::formatOperateOnClassIds(const YAML::Node& tracker) const {
    const std::vector<int> ids = classIds(tracker["class_id"]);
    std::string result;
    if (ids.size() != 1 || ids[0] != -1) {
        for (const int classId : ids) {
            if (!result.empty()) {
                result += ";";
            }
            result += std::to_string(classId);
        }
    }
    return result;
}
