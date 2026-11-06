#include "base_generator/base_event.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

static std::string joinList(const nlohmann::json& value) {
    std::string joined;
    if (value.is_string()) {
        joined = value.get<std::string>();
    } else if (value.is_array()) {
        for (const nlohmann::json& item : value) {
            if (!joined.empty()) {
                joined += ";";
            }
            joined += item.is_string() ? item.get<std::string>() : item.dump();
        }
    }
    return joined;
}

BaseEventGenerator::BaseEventGenerator(std::string pipelineName, nlohmann::json eventCoder, std::optional<nlohmann::json> capturer)
    : pipelineName(std::move(pipelineName)),
      eventCoder(std::move(eventCoder)),
      capturer({{"capture_codes", pipeline::CAPTURE_CODES_DEFAULT}}) {
    if (capturer.has_value()) {
        this->capturer.update(*capturer);
    }
}

std::string BaseEventGenerator::capturerCodes() const {
    const std::string codes = capturer.value("capture_codes", pipeline::CAPTURE_CODES_DEFAULT);
    return codes;
}

std::string BaseEventGenerator::captureOutputDir() const {
    const std::string directory = std::string(pipeline::DEEPSTREAM_OUTPUT_ROOT) + "/" + pipelineName;
    return directory;
}

void BaseEventGenerator::appendEventCoder(const std::string& name) {
    appendNode(
        "presenceeventcoder",
        name,
        addPresenceeventcoder(
            joinList(eventCoder.value("class_ids", nlohmann::json::array())),
            joinList(eventCoder.value("event_names", nlohmann::json::array())),
            eventCoder.value("length", pipeline::PRESENCE_LENGTH),
            eventCoder.value("threshold", pipeline::PRESENCE_THRESHOLD),
            eventCoder.value("mode", std::string(pipeline::PRESENCE_MODE_FOLD))));
}

std::string BaseEventGenerator::afterAnalytics(const std::string& teeName) const {
    (void)teeName;
    const std::string element = "presenceeventcoder";
    return element;
}

void BaseEventGenerator::linkEventCoder(YAML::Node edges, const std::string& teeName) const {
    edges["presenceeventcoder"] = teeName;
}
