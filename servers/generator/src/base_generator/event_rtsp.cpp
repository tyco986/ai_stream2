#include "base_generator/event_rtsp.hpp"

#include "base_generator/config_files.hpp"

#include "constants/generator_pipeline_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/nvtracker_parser.hpp"
#include "subelement_generator/nvdsanalytics.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

static YAML::Node jsonToYaml(const nlohmann::json& value) {
    YAML::Node node;
    if (value.is_object()) {
        node = YAML::Node(YAML::NodeType::Map);
        for (auto item = value.begin(); item != value.end(); ++item) {
            node[item.key()] = jsonToYaml(item.value());
        }
    } else if (value.is_array()) {
        node = YAML::Node(YAML::NodeType::Sequence);
        for (const nlohmann::json& item : value) {
            node.push_back(jsonToYaml(item));
        }
    } else if (value.is_string()) {
        node = value.get<std::string>();
    } else if (value.is_boolean()) {
        node = value.get<bool>();
    } else if (value.is_null()) {
        node = YAML::Node(YAML::NodeType::Null);
    } else if (value.is_number_unsigned()) {
        node = value.get<unsigned int>();
    } else if (value.is_number_integer()) {
        node = value.get<int>();
    } else {
        node = value.get<double>();
    }
    return node;
}

EventRtspGenerator::EventRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : BaseEventGenerator(pipelineName, std::move(eventCoder), std::move(capturer)),
      streams(std::move(streams)),
      analyzer(std::move(analyzer)),
      tracker(std::move(tracker)),
      pgie(std::move(pgie)),
      logger({{"interval", 0}}),
      drawer(std::move(drawer)) {
    if (logger.has_value()) {
        this->logger.update(*logger);
    }
    this->logger["root"] = std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + this->pipelineName;
}

void EventRtspGenerator::init() {
    initStreams();
    initPgie();
    initNvdsanalytics();
    initNvtracker();
    initNvmsgconv();
    initKafka();
    initParams();
    initPipeline();
}

void EventRtspGenerator::initStreams() {
    if (streams.empty()) {
        throw std::invalid_argument("streams cannot be empty");
    }
    std::set<int> widths;
    std::set<int> heights;
    std::set<int> fpsValues;
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        widths.insert(item.value().at("width").get<int>());
        heights.insert(item.value().at("height").get<int>());
        fpsValues.insert(item.value().at("fps").get<int>());
    }
    if (widths.size() != 1) {
        throw std::invalid_argument("streams have inconsistent width");
    }
    if (heights.size() != 1) {
        throw std::invalid_argument("streams have inconsistent height");
    }
    if (fpsValues.size() != 1) {
        throw std::invalid_argument("streams have inconsistent fps");
    }
    width = *widths.begin();
    height = *heights.begin();
    fps = *fpsValues.begin();
    runtimeBatchSize = static_cast<int>(streams.size());
}

void EventRtspGenerator::initParams() {
    const auto pipelineType = generator_pipeline::GENERATOR_PIPELINE_MAP.find(generator);
    if (pipelineType == generator_pipeline::GENERATOR_PIPELINE_MAP.end()) {
        throw std::invalid_argument("unknown generator: " + generator);
    }
    YAML::Node params(YAML::NodeType::Map);
    params["pipeline_name"] = pipelineName;
    params["streams"] = jsonToYaml(streams);
    params["type"] = pipelineType->second;
    params["generator"] = generator;
    params["pgie"] = jsonToYaml(pgie);
    params["analyzer"] = analyzer.has_value() ? jsonToYaml(*analyzer) : YAML::Node(YAML::NodeType::Null);
    params["tracker"] = tracker.has_value() ? jsonToYaml(*tracker) : YAML::Node(YAML::NodeType::Null);
    params["logger"] = jsonToYaml(logger);
    params["drawer"] = drawer.has_value() ? jsonToYaml(*drawer) : YAML::Node(YAML::NodeType::Null);
    params["event_coder"] = jsonToYaml(eventCoder);
    params["capturer"] = jsonToYaml(capturer);
    files.insert_or_assign(PARAMS_CONFIG_NAME, ConfigFile(params));
}

void EventRtspGenerator::initPipeline() {
    add();
    link();
    files.insert_or_assign(PIPELINE_CONFIG_NAME, ConfigFile(pipeline));
}

void EventRtspGenerator::initPgie() {
    const nlohmann::json classAttrs = pgie.at("class_attrs");
    const int interval = pgie.value("interval", 1);
    pgie = nlohmann::json{
        {"model_zip", pgie.at("model_zip")},
        {"class_attrs", classAttrs},
        {"interval", interval},
    };
    pgieConfigParser.emplace(pgie.at("model_zip").get<std::string>(), pipelineName, runtimeBatchSize, jsonToYaml(classAttrs), interval);
    const YAML::Node built = pgieConfigParser->build();
    const YAML::Node property = built["property"];
    YAML::Node mappedClassAttrs(YAML::NodeType::Map);
    for (const auto& section : built) {
        const std::string name = section.first.as<std::string>();
        if (name != "property") {
            mappedClassAttrs[name] = section.second;
        }
    }
    const std::optional<std::string> filterOutClassIds = property["filter-out-class-ids"]
        ? std::optional<std::string>(property["filter-out-class-ids"].as<std::string>())
        : std::nullopt;
    pgieGenerator.emplace(
        property["model-engine-file"].as<std::string>(),
        property["labelfile-path"].as<std::string>(),
        property["batch-size"].as<int>(),
        property["network-mode"].as<int>(),
        property["gpu-id"].as<int>(),
        interval,
        mappedClassAttrs,
        filterOutClassIds,
        property["num-detected-classes"].as<int>());
    applyPgieConfig();
    files.insert_or_assign(PGIE_CONFIG_NAME, ConfigFile(pgieGenerator->config));
}

void EventRtspGenerator::initNvdsanalytics() {
    enableNvdsanalytics = analyzer.has_value();
    YAML::Node analytics = enableNvdsanalytics ? NvdsanalyticsGenerator(jsonToYaml(*analyzer)).config : NvdsanalyticsGenerator().config;
    if (!enableNvdsanalytics) {
        analytics["property"]["config-width"] = width;
        analytics["property"]["config-height"] = height;
    }
    files.insert_or_assign(ANALYTICS_CONFIG_NAME, ConfigFile(analytics));
}

void EventRtspGenerator::initNvtracker() {
    nvtrackerGenerator.reset();
    NvtrackerParser parser(pgie.at("interval").get<int>());
    const YAML::Node trackerNode = tracker.has_value() ? jsonToYaml(*tracker) : YAML::Node(YAML::NodeType::Null);
    enableNvtracker = parser.validate(trackerNode, pgieConfigParser->classIds);
    if (enableNvtracker) {
        nvtrackerGenerator.emplace(parser.maxShadowTrackingAge, parser.earlyTerminationAge, parser.probationAge);
        trackerWidth = parser.alignTrackerDimension(width);
        trackerHeight = parser.alignTrackerDimension(height);
        operateOnClassIds = parser.formatOperateOnClassIds(trackerNode);
        files.insert_or_assign(TRACKER_CONFIG_NAME, ConfigFile(nvtrackerGenerator->config));
    } else {
        files.erase(TRACKER_CONFIG_NAME);
    }
}

std::vector<std::string> EventRtspGenerator::kafkaStreams() const {
    std::vector<std::string> names;
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        names.push_back(item.key());
    }
    return names;
}

void EventRtspGenerator::initNvmsgconv() {
    nvmsgconvGenerator.emplace(kafkaStreams());
    files.insert_or_assign(MSGCONV_CONFIG_NAME, ConfigFile(nvmsgconvGenerator->config));
}

void EventRtspGenerator::initKafka() {
    kafkaGenerator.emplace();
    kafkaTopic = std::string("ai_stream2_") + pipelineName;
    files.insert_or_assign(KAFKA_CONFIG_NAME, ConfigFile(kafkaGenerator->config));
}

void EventRtspGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    for (auto node : pipelineDocument["deepstream"]["nodes"]) {
        const std::string name = node["name"].as<std::string>();
        YAML::Node properties = node["properties"] ? node["properties"] : YAML::Node(YAML::NodeType::Map);
        if (name == "pgie") {
            properties["config-file-path"] = (configSaveDir / PGIE_CONFIG_NAME).string();
        }
        if (name == "nvtracker") {
            properties["ll-config-file"] = (configSaveDir / TRACKER_CONFIG_NAME).string();
        }
        if (name == "nvdsanalytics") {
            properties["config-file"] = (configSaveDir / ANALYTICS_CONFIG_NAME).string();
        }
        if (name == "nvmsgconv") {
            properties["config"] = (configSaveDir / MSGCONV_CONFIG_NAME).string();
        }
        if (name == "nvmsgbroker") {
            properties["config"] = (configSaveDir / KAFKA_CONFIG_NAME).string();
        }
        node["properties"] = properties;
    }
}

std::map<std::string, std::string> EventRtspGenerator::documents(const std::filesystem::path& configSaveDir) {
    YAML::Node pipelineDocument = YAML::Clone(std::get<YAML::Node>(files.at(PIPELINE_CONFIG_NAME)));
    applySavePaths(pipelineDocument, configSaveDir);
    std::map<std::string, std::string> rendered = renderConfigFiles(files, pipelineDocument, PIPELINE_CONFIG_NAME);
    return rendered;
}

void EventRtspGenerator::write(const std::filesystem::path& configSaveDir) {
    std::filesystem::create_directories(configSaveDir);
    const std::filesystem::path nvtrackerSavePath = configSaveDir / TRACKER_CONFIG_NAME;
    if (!enableNvtracker && std::filesystem::exists(nvtrackerSavePath)) {
        std::filesystem::remove(nvtrackerSavePath);
    }
    const std::map<std::string, std::string> rendered = documents(configSaveDir);
    for (const auto& item : rendered) {
        const std::filesystem::path path = configSaveDir / item.first;
        std::ofstream handle;
        handle.exceptions(std::ios::failbit | std::ios::badbit);
        handle.open(path, std::ios::binary);
        handle << item.second;
    }
}

std::string EventRtspGenerator::nvdetDrawerElement() const {
    const std::string element = enableNvtracker ? "nvdetfadedrawerwithtracker" : "nvdetfadedrawer";
    return element;
}

YAML::Node EventRtspGenerator::nvdetDrawerProperties(const nlohmann::json& drawerNode) {
    const int interval = drawerNode.value("interval", 0);
    const int fadeTime = drawerNode.value("fade_time", 0);
    const bool showLabel = drawerNode.value("show_label", false);
    YAML::Node properties = addNvdetfadedrawer(interval, fadeTime, showLabel);
    if (enableNvtracker) {
        properties = addNvdetfadedrawerwithtracker(interval, fadeTime, showLabel, drawerNode.value("show_snap", true));
    }
    return properties;
}

std::string EventRtspGenerator::visualizedSinkUri(const std::string& sourceUri) const {
    const std::string uri = sourceUri + "/" + kafkaTopic;
    return uri;
}

void EventRtspGenerator::appendInfer(int gpuId) {
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(static_cast<int>(streams.size()), width, height, true, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId));
    appendNode("nvinfer", "pgie", addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId));
}

void EventRtspGenerator::linkInfer(YAML::Node edges) {
    edges["nvstreammux"] = "pgie";
}

std::string EventRtspGenerator::inferenceTail() const {
    const std::string tail = "pgie";
    return tail;
}

void EventRtspGenerator::appendAfterOsd(int index, int gpuId) {
    const std::string suffix = std::to_string(index);
    appendNode(
        "nvpresencelogger",
        "nvpresencelogger" + suffix,
        addNvpresencelogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
    appendNode("queue", "queue_vis" + suffix, addQueue());
    appendNode("nvvideoconvert", "nvvideoconvert_vis" + suffix, addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_vis" + suffix, addCapsfilter("video/x-raw(memory:NVMM), format=RGB"));
    appendNode("nvviscapturer", "nvviscapturer" + suffix, addNvviscapturer(captureOutputDir(), capturerCodes()));
    appendNode("fakesink", "fakesink_vis" + suffix, addFakesink(false, false));
}

void EventRtspGenerator::linkAfterOsd(YAML::Node edges, int index) {
    const std::string suffix = std::to_string(index);
    edges["nvosdbin" + suffix] = "nvpresencelogger" + suffix;
    edges["nvpresencelogger" + suffix] = "queue_vis" + suffix;
    edges["queue_vis" + suffix] = "nvvideoconvert_vis" + suffix;
    edges["nvvideoconvert_vis" + suffix] = "capsfilter_vis" + suffix;
    edges["capsfilter_vis" + suffix] = "nvviscapturer" + suffix;
    edges["nvviscapturer" + suffix] = "fakesink_vis" + suffix;
}

void EventRtspGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(item.value().at("url").get<std::string>(), true));
        index += 1;
    }
    appendInfer(gpuId);
    if (enableNvtracker) {
        if (drawer.has_value()) {
            appendNode("nvbboxsnapshot", "nvbboxsnapshot", addNvbboxsnapshot());
        }
        appendNode(
            "nvtracker",
            "nvtracker",
            addNvtracker(subelement::TRACKER_LL_LIB, TRACKER_CONFIG_NAME, trackerWidth, trackerHeight, gpuId, operateOnClassIds));
    }
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME, gpuId));
    appendEventCoder();
    appendNode("tee", "tee_msg", addTee());
    appendNode("queue", "queue_msg", addQueue());
    appendNode("nvmsgconv", "nvmsgconv", addNvmsgconv(MSGCONV_CONFIG_NAME, pipeline::PAYLOAD_DEEPSTREAM_MINIMAL));
    appendNode(
        "nvmsgbroker",
        "nvmsgbroker",
        addNvmsgbroker(subelement::KAFKA_PROTO_LIB, subelement::KAFKA_CONN_STR, kafkaTopic, KAFKA_CONFIG_NAME, false, false));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < static_cast<int>(streams.size()); ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode("nvvideoconvert", "nvvideoconvert" + suffix, addNvvideoconvert(gpuId));
        appendNode("tee", "tee_raw" + suffix, addTee());
        appendNode("queue", "queue_raw" + suffix, addQueue());
        appendNode("nvvideoconvert", "nvvideoconvert_raw" + suffix, addNvvideoconvert(gpuId));
        appendNode("capsfilter", "capsfilter_raw" + suffix, addCapsfilter("video/x-raw(memory:NVMM), format=RGB"));
        appendNode("nvrawcapturer", "nvrawcapturer" + suffix, addNvrawcapturer(captureOutputDir(), capturerCodes()));
        appendNode("fakesink", "fakesink_raw" + suffix, addFakesink(false, false));
        appendNode("queue", "queue_osd" + suffix, addQueue());
        appendNode("nvvideoconvert", "nvvideoconvert_osd" + suffix, addNvvideoconvert(gpuId));
        appendNode("capsfilter", "capsfilter_osd" + suffix, addCapsfilter("video/x-raw(memory:NVMM), format=RGBA"));
        if (drawer.has_value()) {
            appendNode(nvdetDrawerElement(), nvdetDrawerElement() + suffix, nvdetDrawerProperties(*drawer));
        }
        appendNode("nvosdbin", "nvosdbin" + suffix, addNvosdbin(gpuId, true, true));
        appendAfterOsd(streamIndex, gpuId);
    }
}

void EventRtspGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < static_cast<int>(streams.size()); ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    linkInfer(edges);
    std::string tail = inferenceTail();
    if (enableNvtracker) {
        if (drawer.has_value()) {
            edges[tail] = "nvbboxsnapshot";
            edges["nvbboxsnapshot"] = "nvtracker";
        } else {
            edges[tail] = "nvtracker";
        }
        tail = "nvtracker";
    }
    edges[tail] = "nvdsanalytics";
    edges["nvdsanalytics"] = afterAnalytics();
    linkEventCoder(edges);
    edges["tee_msg"].push_back("nvstreamdemux");
    edges["tee_msg"].push_back("queue_msg");
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    for (int index = 0; index < static_cast<int>(streams.size()); ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "nvvideoconvert" + suffix;
        edges["nvvideoconvert" + suffix] = "tee_raw" + suffix;
        edges["tee_raw" + suffix].push_back("queue_raw" + suffix);
        edges["tee_raw" + suffix].push_back("queue_osd" + suffix);
        edges["queue_raw" + suffix] = "nvvideoconvert_raw" + suffix;
        edges["nvvideoconvert_raw" + suffix] = "capsfilter_raw" + suffix;
        edges["capsfilter_raw" + suffix] = "nvrawcapturer" + suffix;
        edges["nvrawcapturer" + suffix] = "fakesink_raw" + suffix;
        edges["queue_osd" + suffix] = "nvvideoconvert_osd" + suffix;
        edges["nvvideoconvert_osd" + suffix] = "capsfilter_osd" + suffix;
        const std::string osdPrev = drawer.has_value() ? nvdetDrawerElement() + suffix : "nvosdbin" + suffix;
        edges["capsfilter_osd" + suffix] = osdPrev;
        if (drawer.has_value()) {
            edges[nvdetDrawerElement() + suffix] = "nvosdbin" + suffix;
        }
        linkAfterOsd(edges, index);
    }
    pipeline["deepstream"]["edges"] = edges;
}
