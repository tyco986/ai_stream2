#include "base_generator/base_rtsp.hpp"

#include "base_generator/config_files.hpp"

#include "constants/generator_pipeline_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/nvtracker_parser.hpp"

#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>
#include <variant>

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

BaseRtspGenerator::BaseRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : pipelineName(std::move(pipelineName)),
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

void BaseRtspGenerator::init() {
    initStreams();
    initPgie();
    initNvdsanalytics();
    initNvtracker();
    initNvmsgconv();
    initKafka();
    initParams();
    initPipeline();
}

void BaseRtspGenerator::initStreams() {
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

void BaseRtspGenerator::initParams() {
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
    files.insert_or_assign(PARAMS_CONFIG_NAME, ConfigFile(params));
}

void BaseRtspGenerator::initPipeline() {
    add();
    link();
    files.insert_or_assign(PIPELINE_CONFIG_NAME, ConfigFile(pipeline));
}

void BaseRtspGenerator::initPgie() {
    const nlohmann::json classAttrs = pgie.at("class_attrs");
    const int interval = pgie.value("interval", 1);
    pgie = nlohmann::json{
        {"model_zip", pgie.at("model_zip")},
        {"class_attrs", classAttrs},
        {"interval", interval},
    };
    pgieConfigParser.emplace(pgie.at("model_zip").get<std::string>(), runtimeBatchSize, jsonToYaml(classAttrs), interval);
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

void BaseRtspGenerator::initNvdsanalytics() {
    enableNvdsanalytics = analyzer.has_value();
    YAML::Node analytics = enableNvdsanalytics ? NvdsanalyticsGenerator(jsonToYaml(*analyzer)).config : NvdsanalyticsGenerator().config;
    if (!enableNvdsanalytics) {
        analytics["property"]["config-width"] = width;
        analytics["property"]["config-height"] = height;
    }
    files.insert_or_assign(ANALYTICS_CONFIG_NAME, ConfigFile(analytics));
}

void BaseRtspGenerator::initNvtracker() {
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

std::vector<std::string> BaseRtspGenerator::kafkaStreams() const {
    std::vector<std::string> names;
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        names.push_back(item.key());
    }
    return names;
}

void BaseRtspGenerator::initNvmsgconv() {
    nvmsgconvGenerator.emplace(kafkaStreams());
    files.insert_or_assign(MSGCONV_CONFIG_NAME, ConfigFile(nvmsgconvGenerator->config));
}

void BaseRtspGenerator::initKafka() {
    kafkaGenerator.emplace();
    kafkaTopic = std::string("ai_stream2_") + pipelineName;
    files.insert_or_assign(KAFKA_CONFIG_NAME, ConfigFile(kafkaGenerator->config));
}

void BaseRtspGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
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

std::map<std::string, std::string> BaseRtspGenerator::documents(const std::filesystem::path& configSaveDir) {
    YAML::Node pipelineDocument = YAML::Clone(std::get<YAML::Node>(files.at(PIPELINE_CONFIG_NAME)));
    applySavePaths(pipelineDocument, configSaveDir);
    std::map<std::string, std::string> rendered = renderConfigFiles(files, pipelineDocument, PIPELINE_CONFIG_NAME);
    return rendered;
}

void BaseRtspGenerator::write(const std::filesystem::path& configSaveDir) {
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

nlohmann::json BaseRtspGenerator::eventOsdKwargs(int gpuId) const {
    const nlohmann::json kwargs = {
        {"gpu_id", gpuId},
        {"display_bbox", true},
        {"display_text", true},
    };
    return kwargs;
}

std::string BaseRtspGenerator::nvdetDrawerElement() const {
    const std::string element = enableNvtracker ? "nvdetfadedrawerwithtracker" : "nvdetfadedrawer";
    return element;
}

std::string BaseRtspGenerator::nvposeDrawerElement() const {
    const std::string element = enableNvtracker ? "nvposefadedrawerwithtracker" : "nvposefadedrawer";
    return element;
}

std::string BaseRtspGenerator::nvsegDrawerElement() const {
    const std::string element = enableNvtracker ? "nvsegfadedrawerwithtracker" : "nvsegfadedrawer";
    return element;
}

YAML::Node BaseRtspGenerator::nvdetDrawerProperties(const nlohmann::json& drawerNode) {
    const int interval = drawerNode.value("interval", 0);
    const int fadeTime = drawerNode.value("fade_time", 0);
    const bool showLabel = drawerNode.value("show_label", false);
    YAML::Node properties = addNvdetfadedrawer(interval, fadeTime, showLabel);
    if (enableNvtracker) {
        properties = addNvdetfadedrawerwithtracker(interval, fadeTime, showLabel, drawerNode.value("show_snap", true));
    }
    return properties;
}

YAML::Node BaseRtspGenerator::nvposeDrawerProperties(const nlohmann::json& drawerNode) {
    const int interval = drawerNode.value("interval", 0);
    const int fadeTime = drawerNode.value("fade_time", 0);
    const bool showLabel = drawerNode.value("show_label", false);
    const bool showPose = drawerNode.value("show_pose", true);
    const double poseThreshold = drawerNode.value("pose_threshold", 0.0);
    const std::string mode = drawerNode.value("mode", std::string(pipeline::POSE_MODE_COCO17));
    YAML::Node properties = addNvposefadedrawer(interval, fadeTime, showLabel, showPose, poseThreshold, mode);
    if (enableNvtracker) {
        properties = addNvposefadedrawerwithtracker(interval, fadeTime, showLabel, showPose, poseThreshold, mode, drawerNode.value("show_snap", true));
    }
    return properties;
}

YAML::Node BaseRtspGenerator::nvsegDrawerProperties(const nlohmann::json& drawerNode) {
    const int interval = drawerNode.value("interval", 0);
    const int fadeTime = drawerNode.value("fade_time", 0);
    const bool showLabel = drawerNode.value("show_label", false);
    const bool showMask = drawerNode.value("show_mask", true);
    YAML::Node properties = addNvsegfadedrawer(interval, fadeTime, showLabel, showMask);
    if (enableNvtracker) {
        properties = addNvsegfadedrawerwithtracker(interval, fadeTime, showLabel, showMask, drawerNode.value("show_snap", true));
    }
    return properties;
}

void BaseRtspGenerator::add() {
    int index = 0;
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(item.value().at("url").get<std::string>(), true));
        index += 1;
    }
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(static_cast<int>(streams.size()), width, height, true, false, pipeline::BATCHED_PUSH_TIMEOUT_US, pgieGenerator->config["property"]["gpu-id"].as<int>()));
    appendNode(
        "nvinfer",
        "pgie",
        addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), pgieGenerator->config["property"]["gpu-id"].as<int>()));
    if (enableNvtracker) {
        if (drawer.has_value()) {
            appendNode("nvbboxsnapshot", "nvbboxsnapshot", addNvbboxsnapshot());
        }
        appendNode(
            "nvtracker",
            "nvtracker",
            addNvtracker(
                subelement::TRACKER_LL_LIB,
                TRACKER_CONFIG_NAME,
                trackerWidth,
                trackerHeight,
                pgieGenerator->config["property"]["gpu-id"].as<int>(),
                operateOnClassIds));
    }
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME, pgieGenerator->config["property"]["gpu-id"].as<int>()));
    appendNode("tee", "tee_msg", addTee());
    appendNode("queue", "queue_msg", addQueue());
    appendNode("nvmsgconv", "nvmsgconv", addNvmsgconv(MSGCONV_CONFIG_NAME, pipeline::PAYLOAD_DEEPSTREAM_MINIMAL));
    appendNode(
        "nvmsgbroker",
        "nvmsgbroker",
        addNvmsgbroker(subelement::KAFKA_PROTO_LIB, subelement::KAFKA_CONN_STR, kafkaTopic, KAFKA_CONFIG_NAME, false, false));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        appendNode("queue", "queue_demux" + std::to_string(streamIndex), addQueue());
        appendNode(
            "nvdetlogger",
            "nvdetlogger" + std::to_string(streamIndex),
            addNvdetlogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
        appendNode("fakesink", "fakesink" + std::to_string(streamIndex), addFakesink(false, false));
    }
}

void BaseRtspGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        edges["nvurisrcbin" + std::to_string(streamIndex)] = "nvstreammux";
    }
    edges["nvstreammux"] = "pgie";
    std::string inferenceTail = "pgie";
    if (enableNvtracker) {
        if (drawer.has_value()) {
            edges[inferenceTail] = "nvbboxsnapshot";
            edges["nvbboxsnapshot"] = "nvtracker";
        } else {
            edges[inferenceTail] = "nvtracker";
        }
        inferenceTail = "nvtracker";
    }
    edges[inferenceTail] = "nvdsanalytics";
    edges["nvdsanalytics"] = "tee_msg";
    edges["tee_msg"].push_back("nvstreamdemux");
    edges["tee_msg"].push_back("queue_msg");
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        edges["queue_demux" + std::to_string(streamIndex)] = "nvdetlogger" + std::to_string(streamIndex);
        edges["nvdetlogger" + std::to_string(streamIndex)] = "fakesink" + std::to_string(streamIndex);
    }
    pipeline["deepstream"]["edges"] = edges;
}

std::string BaseRtspGenerator::visualizedSinkUri(const std::string& sourceUri) const {
    const std::string uri = sourceUri + "/" + kafkaTopic;
    return uri;
}
