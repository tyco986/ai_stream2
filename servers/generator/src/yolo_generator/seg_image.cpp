#include "yolo_generator/seg_image.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_seg.hpp"

#include <utility>

SegImageGenerator::SegImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path filesinkOutput,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(filesinkOutput),
          std::move(analyzer),
          std::move(pgie),
          std::move(logger),
          std::move(drawer)) {
    generator = "SegImageGenerator";
}

void SegImageGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloSeg();
    pgieGenerator->updateConfig();
}

void SegImageGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : images) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.at("path").get<std::string>()), true, 1));
        index += 1;
    }
    appendNode("nvstreammux", "nvstreammux", addNvstreammux(runtimeBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false));
    appendNode("nvinfer", "pgie", addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME));
    appendNode("detcollector", "detcollector", addCollector(logger.value("interval", 1)));
    appendNode("dslogger", "dslogger", addDslogger(pipeline::PAGE_DET));
    const std::string drawerElement = nvsegDrawerElement();
    if (drawer.has_value()) {
        appendNode(drawerElement, drawerElement, nvsegDrawerProperties(*drawer));
    }
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        const std::filesystem::path source(images.at(streamIndex).at("path").get<std::string>());
        const std::filesystem::path sink = filesinkOutput / (source.stem().string() + ".jpg");
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode("nvosdbin", "nvosdbin" + suffix, addNvosdbin(gpuId, true, true, true));
        appendNode("nvvideoconvert", "nvvideoconvert" + suffix, addNvvideoconvert(gpuId));
        appendNode("capsfilter", "capsfilter" + suffix, addCapsfilter());

        appendNode("nvjpegenc", "nvjpegenc" + suffix, addNvjpegenc());
        appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
    }
}

void SegImageGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < runtimeBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "pgie";
    edges["pgie"] = "nvdsanalytics";
    edges["nvdsanalytics"] = "detcollector";
    edges["detcollector"] = "dslogger";
    const std::string drawerElement = nvsegDrawerElement();
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
