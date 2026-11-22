#include "topdown_pose_generator/topdown_pose_image.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

TopdownPoseImageGenerator::TopdownPoseImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path filesinkOutput,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sgie,
    std::optional<nlohmann::json> collector,
    std::optional<nlohmann::json> drawer)
    : BaseImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(filesinkOutput),
          std::move(analyzer),
          std::move(pgie),
          std::move(collector),
          std::move(drawer)),
      sgie(std::move(sgie)) {
    generator = "TopdownPoseImageGenerator";
}

void TopdownPoseImageGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}

void TopdownPoseImageGenerator::initParams() {
    BaseImageGenerator::initParams();
    pose.emplace(sgie, runtimeBatchSize);
    pose->init();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["sgie"] = pose->sgieParams();
    files.insert_or_assign(TopdownPose::SGIE_CONFIG_NAME, ConfigFile(pose->sgieConfig));
    files.insert_or_assign(TopdownPose::RTMPOSE_PREPROCESS_CONFIG_NAME, ConfigFile(pose->preprocessIni));
}

void TopdownPoseImageGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    BaseImageGenerator::applySavePaths(pipelineDocument, configSaveDir);
    pose->applySavePaths(pipelineDocument, configSaveDir);
}

void TopdownPoseImageGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : images) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.at("path").get<std::string>()), true, 1));
        index += 1;
    }
    appendNode("nvstreammux", "nvstreammux", addNvstreammux(runtimeBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false));
    appendNode("nvinfer", "pgie", addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId));
    appendNode("nvdspreprocess", "nvdspreprocess_rtmpose", addNvdspreprocess(TopdownPose::RTMPOSE_PREPROCESS_CONFIG_NAME));
    appendNode("nvinfer", "sgie0", addNvinfer(TopdownPose::SGIE_CONFIG_NAME, pose->batchSize, pose->gpuId, true));
    appendNode("rtmposepostprocess", "nvrtmposepostprocess", addNvrtmposepostprocess(pose->uniqueId));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME));
    appendNode("detcollector", "detcollector", addCollector(collector.value("interval", 1)));
    appendNode("dslogger", "dslogger", addDslogger(pipeline::PAGE_DET));
    const std::string drawerElement = nvposeDrawerElement();
    if (drawer.has_value()) {
        appendNode(drawerElement, drawerElement, nvposeDrawerProperties(*drawer));
    }
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        const std::filesystem::path source(images.at(streamIndex).at("path").get<std::string>());
        const std::filesystem::path sink = filesinkOutput / (source.stem().string() + ".jpg");
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode("nvosdbin", "nvosdbin" + suffix, addNvosdbin(gpuId, true, true, false));
        appendNode("nvvideoconvert", "nvvideoconvert" + suffix, addNvvideoconvert(gpuId));
        appendNode("capsfilter", "capsfilter" + suffix, addCapsfilter());

        appendNode("nvjpegenc", "nvjpegenc" + suffix, addNvjpegenc());
        appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
    }
}

void TopdownPoseImageGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < runtimeBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "pgie";
    edges["pgie"] = "nvdspreprocess_rtmpose";
    edges["nvdspreprocess_rtmpose"] = "sgie0";
    edges["sgie0"] = "nvrtmposepostprocess";
    edges["nvrtmposepostprocess"] = "nvdsanalytics";
    edges["nvdsanalytics"] = "detcollector";
    edges["detcollector"] = "dslogger";
    const std::string drawerElement = nvposeDrawerElement();
    const std::string demuxSource = drawer.has_value() ? drawerElement : std::string("nvstreamdemux");
    if (drawer.has_value()) {
        edges[drawerElement] = "nvstreamdemux";
    }
    edges["dslogger"] = demuxSource;
    for (int index = 0; index < runtimeBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "nvosdbin" + suffix;
        edges["nvosdbin" + suffix] = "nvvideoconvert" + suffix;
        edges["nvvideoconvert" + suffix] = "capsfilter" + suffix;
        edges["capsfilter" + suffix] = "nvjpegenc" + suffix;
        edges["nvjpegenc" + suffix] = "filesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
