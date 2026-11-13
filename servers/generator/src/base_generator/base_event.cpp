#include "base_generator/base_event.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

BaseEventGenerator::BaseEventGenerator(std::string pipelineName, nlohmann::json eventCoder, std::optional<nlohmann::json> capturer)
    : pipelineName(std::move(pipelineName)),
      eventCoder(std::move(eventCoder)),
      capturer({{"capture_codes", pipeline::CAPTURE_CODES_DEFAULT}}) {
    if (capturer.has_value()) {
        this->capturer.update(*capturer);
    }
}

std::string BaseEventGenerator::capturerCodes() const {
    const nlohmann::json value = capturer.contains("capture_codes")
        ? capturer.at("capture_codes")
        : nlohmann::json(pipeline::CAPTURE_CODES_DEFAULT);
    const std::string codes = value.is_string() ? value.get<std::string>() : value.dump();
    return codes;
}

void BaseEventGenerator::appendEventCoder(const std::string& name) {
    appendNode(
        "presenceeventcoder",
        name,
        addPresenceeventcoder(
            eventCoder.value("event_slot_map", std::string()),
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
