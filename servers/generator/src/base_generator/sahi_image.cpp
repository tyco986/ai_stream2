#include "base_generator/sahi_image.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"

#include <utility>

#include <opencv2/imgcodecs.hpp>

SahiImageGenerator::SahiImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(logger),
          std::move(drawer)),
      sahi(std::move(sahi)) {
}

void SahiImageGenerator::initImages() {
    BaseImageGenerator::initImages();
    muxBatchSize = runtimeBatchSize;
    const nlohmann::json& preprocess = sahi.at("nvsahipreprocess");
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

void SahiImageGenerator::initParams() {
    BaseImageGenerator::initParams();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["sahi"] = YAML::Load(sahi.dump());
}

void SahiImageGenerator::initPipeline() {
    initSahi();
    BaseImageGenerator::initPipeline();
}

void SahiImageGenerator::initSahi() {
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

void SahiImageGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    BaseImageGenerator::applySavePaths(pipelineDocument, configSaveDir);
    for (auto node : pipelineDocument["deepstream"]["nodes"]) {
        if (node["name"].as<std::string>() == "nvsahipreprocess") {
            YAML::Node properties = node["properties"] ? node["properties"] : YAML::Node(YAML::NodeType::Map);
            properties["config-file"] = (configSaveDir / SAHI_PREPROCESS_CONFIG_NAME).string();
            node["properties"] = properties;
        }
    }
}

YAML::Node SahiImageGenerator::sahiPostprocessProperties(const nlohmann::json& postprocess) {
    const std::string gieId = std::to_string(pgieGenerator->config["property"]["gie-unique-id"].as<int>());
    const YAML::Node properties = addNvsahipostprocess(
        gieId,
        pipeline::MATCH_METRIC_IOS,
        postprocess.at("match_threshold").get<double>(),
        false,
        postprocess.value("enable_merge", true),
        true);
    return properties;
}

void SahiImageGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : images) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.get<std::string>()), true, 1));
        index += 1;
    }
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(muxBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId));
    const nlohmann::json& preprocess = sahi.at("nvsahipreprocess");
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
    appendNode(sahiPostprocess, sahiPostprocess, sahiPostprocessProperties(sahi.at(sahiPostprocess)));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME, gpuId));
    appendNode("tee", "tee_msg", addTee());
    appendNode("queue", "queue_msg", addQueue());
    appendNode("nvmsgconv", "nvmsgconv", addNvmsgconv(MSGCONV_CONFIG_NAME, pipeline::PAYLOAD_DEEPSTREAM_MINIMAL));
    appendNode(
        "nvmsgbroker",
        "nvmsgbroker",
        addNvmsgbroker(subelement::KAFKA_PROTO_LIB, subelement::KAFKA_CONN_STR, kafkaTopic, KAFKA_CONFIG_NAME, false, false));
    if (drawer.has_value()) {
        appendNode(visDrawerElement(), visDrawerElement(), visDrawerProperties(*drawer));
    }
    appendNode("nvosdbin", "nvosdbin", addNvosdbin(gpuId, true, true));
    appendNode("nvvideoconvert", "nvvideoconvert", addNvvideoconvert(gpuId));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < muxBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        const std::filesystem::path source(images.at(streamIndex).get<std::string>());
        const std::filesystem::path sink = output / (suffix + "_" + source.stem().string() + ".jpg");
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode("capsfilter", "capsfilter" + suffix, addCapsfilter());
        appendNode(
            "detlogger",
            "detlogger" + suffix,
            addDetlogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
        appendNode("nvjpegenc", "nvjpegenc" + suffix, addNvjpegenc());
        appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
    }
}

void SahiImageGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < muxBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "nvsahipreprocess";
    edges["nvsahipreprocess"] = "pgie";
    edges["pgie"] = "queue_sahi";
    edges["queue_sahi"] = sahiPostprocess;
    edges[sahiPostprocess] = "nvdsanalytics";
    edges["nvdsanalytics"] = "tee_msg";
    const std::string visNext = drawer.has_value() ? visDrawerElement() : "nvosdbin";
    edges["tee_msg"].push_back(visNext);
    edges["tee_msg"].push_back("queue_msg");
    if (drawer.has_value()) {
        edges[visDrawerElement()] = "nvosdbin";
    }
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    edges["nvosdbin"] = "nvvideoconvert";
    edges["nvvideoconvert"] = "nvstreamdemux";
    for (int index = 0; index < muxBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "capsfilter" + suffix;
        edges["capsfilter" + suffix] = "detlogger" + suffix;
        edges["detlogger" + suffix] = "nvjpegenc" + suffix;
        edges["nvjpegenc" + suffix] = "filesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}

std::string SahiImageGenerator::visDrawerElement() const {
    const std::string element = "detfadedrawer";
    return element;
}

YAML::Node SahiImageGenerator::visDrawerProperties(const nlohmann::json& drawerNode) {
    const YAML::Node properties = nvdetDrawerProperties(drawerNode);
    return properties;
}
