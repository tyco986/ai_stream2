#include "base_generator/event_video.hpp"

#include "base_generator/config_files.hpp"

#include "constants/generator_pipeline_const.hpp"
#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/nvtracker_parser.hpp"

#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <opencv2/videoio.hpp>

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

static void probeVideo(const std::filesystem::path& path, int& width, int& height, int& fps) {
    cv::VideoCapture capture(path.string());
    if (!capture.isOpened()) {
        throw std::runtime_error("input video not found: " + path.string());
    }
    width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    fps = static_cast<int>(std::lround(capture.get(cv::CAP_PROP_FPS)));
    if (width <= 0 || height <= 0 || fps <= 0) {
        throw std::runtime_error("invalid video metadata: " + path.string());
    }
}

std::string EventVideoGenerator::fileUri(const std::filesystem::path& path) {
    const std::filesystem::path absolute = std::filesystem::absolute(path);
    if (!std::filesystem::is_regular_file(absolute)) {
        throw std::invalid_argument("input video not found: " + absolute.string());
    }
    std::string uri = "file://";
    const std::string text = absolute.generic_string();
    for (const unsigned char character : text) {
        const bool plain = std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~' || character == '/';
        if (plain) {
            uri.push_back(static_cast<char>(character));
        } else {
            constexpr char hex[] = "0123456789ABCDEF";
            uri.push_back('%');
            uri.push_back(hex[character >> 4]);
            uri.push_back(hex[character & 0x0f]);
        }
    }
    return uri;
}

EventVideoGenerator::EventVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : BaseEventGenerator(pipelineName, std::move(eventCoder), std::move(capturer)),
      videos(std::move(videos)),
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

void EventVideoGenerator::init() {
    initVideos();
    initPgie();
    initNvdsanalytics();
    initNvtracker();
    initNvmsgconv();
    initKafka();
    initParams();
    initPipeline();
}

void EventVideoGenerator::initVideos() {
    if (!videos.is_array() || videos.empty()) {
        throw std::invalid_argument("videos cannot be empty");
    }
    videoFps.clear();
    int index = 0;
    for (const nlohmann::json& item : videos) {
        int videoWidth = 0;
        int videoHeight = 0;
        int probedFps = 0;
        probeVideo(item.get<std::string>(), videoWidth, videoHeight, probedFps);
        if (index == 0) {
            width = videoWidth;
            height = videoHeight;
            fps = probedFps;
        } else if (videoWidth != width || videoHeight != height) {
            throw std::invalid_argument("videos have inconsistent resolution");
        }
        videoFps.push_back(probedFps);
        index += 1;
    }
    runtimeBatchSize = static_cast<int>(videos.size());
}

void EventVideoGenerator::initParams() {
    const auto pipelineType = generator_pipeline::GENERATOR_PIPELINE_MAP.find(generator);
    if (pipelineType == generator_pipeline::GENERATOR_PIPELINE_MAP.end()) {
        throw std::invalid_argument("unknown generator: " + generator);
    }
    YAML::Node params(YAML::NodeType::Map);
    params["pipeline_name"] = pipelineName;
    params["videos"] = jsonToYaml(videos);
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

void EventVideoGenerator::initPipeline() {
    add();
    link();
    files.insert_or_assign(PIPELINE_CONFIG_NAME, ConfigFile(pipeline));
}

void EventVideoGenerator::initPgie() {
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

void EventVideoGenerator::initNvdsanalytics() {
    enableNvdsanalytics = analyzer.has_value();
    YAML::Node analytics = enableNvdsanalytics ? NvdsanalyticsGenerator(jsonToYaml(*analyzer)).config : NvdsanalyticsGenerator().config;
    if (!enableNvdsanalytics) {
        analytics["property"]["config-width"] = width;
        analytics["property"]["config-height"] = height;
    }
    files.insert_or_assign(ANALYTICS_CONFIG_NAME, ConfigFile(analytics));
}

void EventVideoGenerator::initNvtracker() {
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

std::vector<std::string> EventVideoGenerator::kafkaStreams() const {
    std::vector<std::string> names;
    for (const nlohmann::json& item : videos) {
        names.push_back(item.get<std::string>());
    }
    return names;
}

void EventVideoGenerator::initNvmsgconv() {
    nvmsgconvGenerator.emplace(kafkaStreams());
    files.insert_or_assign(MSGCONV_CONFIG_NAME, ConfigFile(nvmsgconvGenerator->config));
}

void EventVideoGenerator::initKafka() {
    kafkaGenerator.emplace();
    kafkaTopic = std::string("ai_stream2_") + pipelineName;
    files.insert_or_assign(KAFKA_CONFIG_NAME, ConfigFile(kafkaGenerator->config));
}

void EventVideoGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
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

std::map<std::string, std::string> EventVideoGenerator::documents(const std::filesystem::path& configSaveDir) {
    YAML::Node pipelineDocument = YAML::Clone(std::get<YAML::Node>(files.at(PIPELINE_CONFIG_NAME)));
    applySavePaths(pipelineDocument, configSaveDir);
    std::map<std::string, std::string> rendered = renderConfigFiles(files, pipelineDocument, PIPELINE_CONFIG_NAME);
    return rendered;
}

void EventVideoGenerator::write(const std::filesystem::path& configSaveDir) {
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

std::string EventVideoGenerator::nvdetDrawerElement() const {
    const std::string element = enableNvtracker ? "nvdetfadedrawerwithtracker" : "nvdetfadedrawer";
    return element;
}

YAML::Node EventVideoGenerator::nvdetDrawerProperties(const nlohmann::json& drawerNode) {
    const int interval = drawerNode.value("interval", 0);
    const int fadeTime = drawerNode.value("fade_time", 0);
    const bool showLabel = drawerNode.value("show_label", false);
    YAML::Node properties = addNvdetfadedrawer(interval, fadeTime, showLabel);
    if (enableNvtracker) {
        properties = addNvdetfadedrawerwithtracker(interval, fadeTime, showLabel, drawerNode.value("show_snap", true));
    }
    return properties;
}

void EventVideoGenerator::appendInfer(int gpuId) {
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(runtimeBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false));
    appendNode("nvinfer", "pgie", addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId));
}

void EventVideoGenerator::linkInfer(YAML::Node edges) {
    edges["nvstreammux"] = "pgie";
}

void EventVideoGenerator::appendEncode(int gpuId) {
    (void)gpuId;
    appendNode("fakesink", "fakesink", addFakesink(false, false));
}

void EventVideoGenerator::linkEncode(YAML::Node edges) {
    edges["nvpresencelogger"] = "fakesink";
}

std::string EventVideoGenerator::inferenceTail() const {
    const std::string tail = "pgie";
    return tail;
}

void EventVideoGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : videos) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.get<std::string>()), true));
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
    appendNode("nvvideoconvert", "nvvideoconvert", addNvvideoconvert(gpuId));
    appendNode("tee", "tee_raw", addTee());
    appendNode("queue", "queue_raw", addQueue());
    appendNode("nvvideoconvert", "nvvideoconvert_raw", addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_raw", addCapsfilter("video/x-raw(memory:NVMM), format=RGB"));
    appendNode("nvrawcapturer", "nvrawcapturer", addNvrawcapturer(captureOutputDir(), capturerCodes()));
    appendNode("fakesink", "fakesink_raw", addFakesink(false, false));
    appendNode("queue", "queue_osd", addQueue());
    appendNode("nvvideoconvert", "nvvideoconvert_osd", addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_osd", addCapsfilter("video/x-raw(memory:NVMM), format=RGBA"));
    if (drawer.has_value()) {
        appendNode(nvdetDrawerElement(), nvdetDrawerElement(), nvdetDrawerProperties(*drawer));
    }
    appendNode("nvosdbin", "nvosdbin", addNvosdbin(gpuId, true, true));
    appendNode("tee", "tee_vis", addTee());
    appendNode("queue", "queue_vis", addQueue());
    appendNode("nvvideoconvert", "nvvideoconvert_vis", addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_vis", addCapsfilter("video/x-raw(memory:NVMM), format=RGB"));
    appendNode("nvviscapturer", "nvviscapturer", addNvviscapturer(captureOutputDir(), capturerCodes()));
    appendNode("fakesink", "fakesink_vis", addFakesink(false, false));
    appendNode("queue", "queue_enc", addQueue());
    appendNode(
        "nvpresencelogger",
        "nvpresencelogger",
        addNvpresencelogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
    appendEncode(gpuId);
}

void EventVideoGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < static_cast<int>(videos.size()); ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    linkInfer(edges);
    std::string inferenceTail = this->inferenceTail();
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
    edges["nvdsanalytics"] = afterAnalytics();
    linkEventCoder(edges);
    edges["tee_msg"].push_back("nvvideoconvert");
    edges["tee_msg"].push_back("queue_msg");
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    edges["nvvideoconvert"] = "tee_raw";
    edges["tee_raw"].push_back("queue_raw");
    edges["tee_raw"].push_back("queue_osd");
    edges["queue_raw"] = "nvvideoconvert_raw";
    edges["nvvideoconvert_raw"] = "capsfilter_raw";
    edges["capsfilter_raw"] = "nvrawcapturer";
    edges["nvrawcapturer"] = "fakesink_raw";
    edges["queue_osd"] = "nvvideoconvert_osd";
    edges["nvvideoconvert_osd"] = "capsfilter_osd";
    const std::string osdPrev = drawer.has_value() ? nvdetDrawerElement() : "nvosdbin";
    edges["capsfilter_osd"] = osdPrev;
    if (drawer.has_value()) {
        edges[nvdetDrawerElement()] = "nvosdbin";
    }
    edges["nvosdbin"] = "tee_vis";
    edges["tee_vis"].push_back("queue_vis");
    edges["tee_vis"].push_back("queue_enc");
    edges["queue_vis"] = "nvvideoconvert_vis";
    edges["nvvideoconvert_vis"] = "capsfilter_vis";
    edges["capsfilter_vis"] = "nvviscapturer";
    edges["nvviscapturer"] = "fakesink_vis";
    edges["queue_enc"] = "nvpresencelogger";
    linkEncode(edges);
    pipeline["deepstream"]["edges"] = edges;
}
