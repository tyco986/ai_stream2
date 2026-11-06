#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_vis_video.hpp"

#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

StgcnppRtmposeVisVideoGenerator::StgcnppRtmposeVisVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sgie,
    nlohmann::json stgcnpp,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : VisVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)),
      sgie(std::move(sgie)),
      stgcnpp(std::move(stgcnpp)) {
    generator = "StgcnppRtmposeVisVideoGenerator";
}

void StgcnppRtmposeVisVideoGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}

void StgcnppRtmposeVisVideoGenerator::initParams() {
    BaseVideoGenerator::initParams();
    pose.emplace(sgie, runtimeBatchSize);
    pose->init();
    model.emplace(stgcnpp, runtimeBatchSize);
    model->init(pgie.at("interval").get<int>());
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["sgie"] = pose->sgieParams();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["stgcnpp"] = model->params();
    files.insert_or_assign(TopdownPose::SGIE_CONFIG_NAME, ConfigFile(pose->sgieConfig));
    files.insert_or_assign(TopdownPose::RTMPOSE_PREPROCESS_CONFIG_NAME, ConfigFile(pose->preprocessIni));
    files.insert_or_assign(StgcnppYoloPose::CONFIG_NAME, ConfigFile(model->config));
    files.insert_or_assign(StgcnppYoloPose::PREPROCESS_CONFIG_NAME, ConfigFile(model->preprocessIni));
    files.insert_or_assign(StgcnppYoloPose::META_NAME, ConfigFile(model->metaText));
}

void StgcnppRtmposeVisVideoGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    BaseVideoGenerator::applySavePaths(pipelineDocument, configSaveDir);
    pose->applySavePaths(pipelineDocument, configSaveDir);
    model->applySavePaths(pipelineDocument, configSaveDir);
}

void StgcnppRtmposeVisVideoGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : videos) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.get<std::string>()), true));
        index += 1;
    }
    YAML::Node mux = addNvstreammux(runtimeBatchSize, width, height, false, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false);
    appendNode("nvstreammux", "nvstreammux", mux);
    appendNode("nvinfer", "pgie", addNvinfer(PGIE_CONFIG_NAME, pgieGenerator->config["property"]["batch-size"].as<int>(), gpuId));
    if (enableNvtracker) {
        if (drawer.has_value()) {
            appendNode("boxsnapshot", "boxsnapshot", addBoxsnapshot());
        }
        appendNode(
            "nvtracker",
            "nvtracker",
            addNvtracker(subelement::TRACKER_LL_LIB, TRACKER_CONFIG_NAME, trackerWidth, trackerHeight, gpuId, operateOnClassIds));
    }
    appendNode("nvdspreprocess", "nvdspreprocess_rtmpose", addNvdspreprocess(TopdownPose::RTMPOSE_PREPROCESS_CONFIG_NAME));
    appendNode("nvinfer", "sgie0", addNvinfer(TopdownPose::SGIE_CONFIG_NAME, pose->batchSize, pose->gpuId, true));
    appendNode("nvrtmposepostprocess", "nvrtmposepostprocess", addNvrtmposepostprocess(pose->uniqueId));
    appendNode("nvdspreprocess", "nvdspreprocess", addNvdspreprocess(StgcnppYoloPose::PREPROCESS_CONFIG_NAME));
    appendNode("nvinfer", "sgie1", addNvinfer(StgcnppYoloPose::CONFIG_NAME, model->batchSize, model->gpuId, true));
    appendNode("stgcnppactionlabel", "stgcnppactionlabel", addStgcnppactionlabel(model->uniqueId));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME, gpuId));
    appendNode("tee", "tee_msg", addTee());
    appendNode("queue", "queue_msg", addQueue());
    appendNode("nvmsgconv", "nvmsgconv", addNvmsgconv(MSGCONV_CONFIG_NAME, pipeline::PAYLOAD_DEEPSTREAM_MINIMAL));
    appendNode(
        "nvmsgbroker",
        "nvmsgbroker",
        addNvmsgbroker(subelement::KAFKA_PROTO_LIB, subelement::KAFKA_CONN_STR, kafkaTopic, KAFKA_CONFIG_NAME, false, false));
    const std::string drawerElement = std::string(StgcnppYoloPose::DRAWER_ELEMENT);
    if (drawer.has_value()) {
        appendNode(drawerElement, drawerElement, StgcnppYoloPose::drawerProperties(*drawer));
    }
    appendNode("nvosdbin", "nvosdbin", addNvosdbin(gpuId, true, true, false));
    appendNode("nvvideoconvert", "nvvideoconvert", addNvvideoconvert(gpuId));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        const std::filesystem::path source(videos.at(streamIndex).get<std::string>());
        const std::filesystem::path sink = output / (suffix + "_" + source.stem().string() + ".mp4");
        appendNode("queue", "queue" + suffix, addQueue());
        appendNode(
            "stgcnpplogger",
            "stgcnpplogger" + suffix,
            addStgcnpplogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
        appendNode(
            "nvv4l2h264enc",
            "nvv4l2h264enc" + suffix,
            addNvv4l2h264enc(pipeline::H264_BITRATE, videoFps[streamIndex], pipeline::H264_PRESET_ID, gpuId));
        appendNode("h264parse", "h264parse" + suffix, addH264parse());
        appendNode("mp4mux", "mp4mux" + suffix, addMp4mux());
        appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
    }
}

void StgcnppRtmposeVisVideoGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < runtimeBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "pgie";
    std::string inferenceTail = "pgie";
    if (enableNvtracker) {
        if (drawer.has_value()) {
            edges[inferenceTail] = "boxsnapshot";
            edges["boxsnapshot"] = "nvtracker";
        } else {
            edges[inferenceTail] = "nvtracker";
        }
        inferenceTail = "nvtracker";
    }
    edges[inferenceTail] = "nvdspreprocess_rtmpose";
    edges["nvdspreprocess_rtmpose"] = "sgie0";
    edges["sgie0"] = "nvrtmposepostprocess";
    edges["nvrtmposepostprocess"] = "nvdspreprocess";
    edges["nvdspreprocess"] = "sgie1";
    edges["sgie1"] = "stgcnppactionlabel";
    edges["stgcnppactionlabel"] = "nvdsanalytics";
    edges["nvdsanalytics"] = "tee_msg";
    const std::string drawerElement = std::string(StgcnppYoloPose::DRAWER_ELEMENT);
    const std::string visNext = drawer.has_value() ? drawerElement : "nvosdbin";
    edges["tee_msg"].push_back(visNext);
    edges["tee_msg"].push_back("queue_msg");
    if (drawer.has_value()) {
        edges[drawerElement] = "nvosdbin";
    }
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    edges["nvosdbin"] = "nvvideoconvert";
    edges["nvvideoconvert"] = "nvstreamdemux";
    for (int index = 0; index < runtimeBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue" + suffix] = "stgcnpplogger" + suffix;
        edges["stgcnpplogger" + suffix] = "nvv4l2h264enc" + suffix;
        edges["nvv4l2h264enc" + suffix] = "h264parse" + suffix;
        edges["h264parse" + suffix] = "mp4mux" + suffix;
        edges["mp4mux" + suffix] = "filesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
