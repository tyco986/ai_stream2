#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_video.hpp"

#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

StgcnppRtmposeVideoGenerator::StgcnppRtmposeVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sgie,
    nlohmann::json stgcnpp,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)),
      sgie(std::move(sgie)),
      stgcnpp(std::move(stgcnpp)) {
    generator = "StgcnppRtmposeVideoGenerator";
}

void StgcnppRtmposeVideoGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}

void StgcnppRtmposeVideoGenerator::initParams() {
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

void StgcnppRtmposeVideoGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    BaseVideoGenerator::applySavePaths(pipelineDocument, configSaveDir);
    pose->applySavePaths(pipelineDocument, configSaveDir);
    model->applySavePaths(pipelineDocument, configSaveDir);
}

void StgcnppRtmposeVideoGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (const nlohmann::json& item : videos) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(fileUri(item.get<std::string>()), true));
        index += 1;
    }
    YAML::Node mux = addNvstreammux(runtimeBatchSize, width, height, true, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId, false);
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
    appendNode("rtmposepostprocess", "nvrtmposepostprocess", addNvrtmposepostprocess(pose->uniqueId));
    appendNode("nvdspreprocess", "nvdspreprocess", addNvdspreprocess(StgcnppYoloPose::PREPROCESS_CONFIG_NAME));
    appendNode("nvinfer", "sgie1", addNvinfer(StgcnppYoloPose::CONFIG_NAME, model->batchSize, model->gpuId, true));
    appendNode("stgcnppactionlabel", "stgcnppactionlabel", addStgcnppactionlabel(model->uniqueId));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME));
    appendNode(
        "stgcnpplogger",
        "stgcnpplogger",
        addStgcnpplogger(logger.value("interval", 0)));
    appendNode("fakesink", "fakesink", addFakesink(false, false));
}

void StgcnppRtmposeVideoGenerator::link() {
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
    edges["nvdsanalytics"] = "stgcnpplogger";
    edges["stgcnpplogger"] = "fakesink";
    pipeline["deepstream"]["edges"] = edges;
}
