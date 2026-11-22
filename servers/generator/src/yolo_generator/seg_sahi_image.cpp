#include "yolo_generator/seg_sahi_image.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_seg.hpp"

#include <utility>

SegSahiImageGenerator::SegSahiImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path filesinkOutput,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> collector,
    std::optional<nlohmann::json> drawer)
    : SahiImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(filesinkOutput),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(collector),
          std::move(drawer)) {
    generator = "SegSahiImageGenerator";
}

void SegSahiImageGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloSeg();
    pgieGenerator->updateConfig();
}

void SegSahiImageGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : images) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.at("path").get<std::string>()), true, 1));
        index += 1;
    }
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(muxBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false));
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
    appendNode(sahiPostprocess, sahiPostprocess, sahiPostprocessProperties(sahi.at("postprocess")));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME));
    appendNode("detcollector", "detcollector", addCollector(collector.value("interval", 1)));
    appendNode("dslogger", "dslogger", addDslogger(pipeline::PAGE_DET));
    const std::string drawerElement = nvsegDrawerElement();
    if (drawer.has_value()) {
        appendNode(drawerElement, drawerElement, nvsegDrawerProperties(*drawer));
    }
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < muxBatchSize; ++streamIndex) {
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

void SegSahiImageGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < muxBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "nvsahipreprocess";
    edges["nvsahipreprocess"] = "pgie";
    edges["pgie"] = "queue_sahi";
    edges["queue_sahi"] = sahiPostprocess;
    edges[sahiPostprocess] = "nvdsanalytics";
    edges["nvdsanalytics"] = "detcollector";
    edges["detcollector"] = "dslogger";
    const std::string drawerElement = nvsegDrawerElement();
    const std::string demuxSource = drawer.has_value() ? drawerElement : std::string("nvstreamdemux");
    if (drawer.has_value()) {
        edges[drawerElement] = "nvstreamdemux";
    }
    edges["dslogger"] = demuxSource;
    for (int index = 0; index < muxBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "nvosdbin" + suffix;
        edges["nvosdbin" + suffix] = "nvvideoconvert" + suffix;
        edges["nvvideoconvert" + suffix] = "capsfilter" + suffix;
        edges["capsfilter" + suffix] = "nvjpegenc" + suffix;
        edges["nvjpegenc" + suffix] = "filesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
