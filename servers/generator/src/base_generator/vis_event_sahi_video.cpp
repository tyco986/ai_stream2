#include "base_generator/vis_event_sahi_video.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

#include <opencv2/imgcodecs.hpp>

VisEventSahiVideoGenerator::VisEventSahiVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : VisEventVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(eventCoder),
          std::move(tracker),
          std::move(logger),
          std::move(drawer),
          std::move(capturer)),
      sahi(std::move(sahi)) {
}

void VisEventSahiVideoGenerator::initVideos() {
    VisEventVideoGenerator::initVideos();
    muxBatchSize = runtimeBatchSize;
    const nlohmann::json& preprocess = sahi.at("preprocess");
    sahiInfo = Sahi().boxes(
        width,
        height,
        preprocess.at("slice_width").get<int>(),
        preprocess.at("slice_height").get<int>(),
        preprocess.at("overlap_width_ratio").get<double>(),
        preprocess.at("overlap_height_ratio").get<double>(),
        preprocess.value("enable_full_frame", true));
    runtimeBatchSize = sahiInfo.num;
}

void VisEventSahiVideoGenerator::initParams() {
    VisEventVideoGenerator::initParams();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["sahi"] = YAML::Load(sahi.dump());
}

void VisEventSahiVideoGenerator::initPipeline() {
    initSahi();
    VisEventVideoGenerator::initPipeline();
}

void VisEventSahiVideoGenerator::initSahi() {
    const nlohmann::json& shape = pgieConfigParser->inputShape();
    std::string networkInputShape = std::to_string(runtimeBatchSize);
    for (std::size_t index = 1; index < shape.size(); ++index) {
        networkInputShape += ";" + std::to_string(shape.at(index).get<int>());
    }
    const int gieUniqueId = pgieGenerator->config["property"]["gie-unique-id"].as<int>();
    nvsahipreprocessGenerator.emplace(
        networkInputShape,
        gieUniqueId,
        0,
        pgieConfigParser->inputName());
    std::string configText;
    for (const std::string& line : nvsahipreprocessGenerator->config) {
        configText += line;
        configText += "\n";
    }
    files.insert_or_assign(SAHI_PREPROCESS_CONFIG_NAME, ConfigFile(configText));
    nlohmann::json info = {
        {"width", sahiInfo.width},
        {"height", sahiInfo.height},
        {"row", sahiInfo.row},
        {"column", sahiInfo.column},
        {"num", sahiInfo.num},
    };
    for (const auto& item : sahiInfo.box) {
        info["box"][item.first] = {
            {"x1", item.second.x1},
            {"y1", item.second.y1},
            {"x2", item.second.x2},
            {"y2", item.second.y2},
        };
    }
    files.insert_or_assign(SAHI_INFO_NAME, ConfigFile(info.dump(2)));
    std::vector<unsigned char> encoded;
    cv::imencode(".jpg", Sahi().preview(sahiInfo), encoded);
    files.insert_or_assign(SAHI_PREVIEW_NAME, ConfigFile(std::string(encoded.begin(), encoded.end())));
}

void VisEventSahiVideoGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    EventVideoGenerator::applySavePaths(pipelineDocument, configSaveDir);
    for (auto node : pipelineDocument["deepstream"]["nodes"]) {
        if (node["name"].as<std::string>() == "nvsahipreprocess") {
            YAML::Node properties = node["properties"] ? node["properties"] : YAML::Node(YAML::NodeType::Map);
            properties["config-file"] = (configSaveDir / SAHI_PREPROCESS_CONFIG_NAME).string();
            node["properties"] = properties;
        }
    }
}

void VisEventSahiVideoGenerator::appendInfer(int gpuId) {
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(muxBatchSize, width, height, true, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false));
    const nlohmann::json& preprocess = sahi.at("preprocess");
    appendNode(
        "nvsahipreprocess",
        "nvsahipreprocess",
        addNvsahipreprocess(
            SAHI_PREPROCESS_CONFIG_NAME,
            preprocess.at("slice_width").get<int>(),
            preprocess.at("slice_height").get<int>(),
            preprocess.at("overlap_width_ratio").get<double>(),
            preprocess.at("overlap_height_ratio").get<double>(),
            preprocess.value("enable_full_frame", true),
            gpuId));
    appendNode(
        "nvinfer",
        "pgie",
        addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId, true));
    appendNode("queue", "queue_sahi", addQueue());
    const nlohmann::json& postprocess = sahi.at("postprocess");
    const std::string gieId = std::to_string(pgieGenerator->config["property"]["gie-unique-id"].as<int>());
    appendNode(
        SAHI_POSTPROCESS,
        SAHI_POSTPROCESS,
        addNvsahipostprocess(
            gieId,
            postprocess.value("match_metric", pipeline::MATCH_METRIC_IOS),
            postprocess.at("match_threshold").get<double>(),
            postprocess.value("class_agnostic", false),
            postprocess.value("enable_merge", true),
            postprocess.value("two_phase_nmm", true),
            postprocess.value("prefer_full_frame", true)));
}

void VisEventSahiVideoGenerator::linkInfer(YAML::Node edges) {
    edges["nvstreammux"] = "nvsahipreprocess";
    edges["nvsahipreprocess"] = "pgie";
    edges["pgie"] = "queue_sahi";
    edges["queue_sahi"] = SAHI_POSTPROCESS;
}

std::string VisEventSahiVideoGenerator::inferenceTail() const {
    const std::string tail = SAHI_POSTPROCESS;
    return tail;
}
