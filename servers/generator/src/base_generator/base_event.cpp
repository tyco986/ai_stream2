#include "base_generator/base_event.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

BaseEventGenerator::BaseEventGenerator(
    std::string pipelineName,
    std::filesystem::path capturerOutput,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> capturer)
    : pipelineName(std::move(pipelineName)),
      capturerOutput(std::filesystem::absolute(std::move(capturerOutput))),
      eventCoder(std::move(eventCoder)),
      capturer({{pipeline::KEY_LABEL_TASK, pipeline::LABEL_TASK_DET}}) {
    if (capturer.has_value()) {
        this->capturer.update(*capturer);
    }
}

std::string BaseEventGenerator::dumpCodes(const nlohmann::json& collector) const {
    const nlohmann::json value = collector.contains(pipeline::KEY_DUMP_CODES)
        ? collector.at(pipeline::KEY_DUMP_CODES)
        : nlohmann::json(pipeline::DUMP_CODES_DEFAULT);
    const std::string codes = value.is_string() ? value.get<std::string>() : value.dump();
    return codes;
}

std::string BaseEventGenerator::labelTask() const {
    const nlohmann::json value = capturer.contains(pipeline::KEY_LABEL_TASK)
        ? capturer.at(pipeline::KEY_LABEL_TASK)
        : nlohmann::json(pipeline::LABEL_TASK_DET);
    const std::string task = value.is_string() ? value.get<std::string>() : value.dump();
    return task;
}

void BaseEventGenerator::appendEventCoder(int interval, const nlohmann::json& collector, const std::string& name) {
    appendNode(
        "presenceeventcoder",
        name,
        addPresenceeventcoder(
            eventCoder.value("event_slot_map", std::string()),
            eventCoder.value("length", pipeline::PRESENCE_LENGTH),
            eventCoder.value("threshold", pipeline::PRESENCE_THRESHOLD),
            eventCoder.value("mode", std::string(pipeline::PRESENCE_MODE_FOLD))));
    appendNode("presencecollector", "presencecollector", addPresenceCollector(interval, dumpCodes(collector), capturerOutput.string()));
    appendNode("dslogger", "dslogger", addDslogger(pipeline::PAGE_PRESENCE));
}

std::string BaseEventGenerator::afterAnalytics(const std::string& teeName) const {
    (void)teeName;
    const std::string element = "presenceeventcoder";
    return element;
}

void BaseEventGenerator::linkEventCoder(YAML::Node edges, const std::string& teeName) const {
    edges["presenceeventcoder"] = "presencecollector";
    edges["presencecollector"] = "dslogger";
    edges["dslogger"] = teeName;
}
