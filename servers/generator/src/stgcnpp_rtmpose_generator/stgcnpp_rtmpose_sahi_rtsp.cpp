#include "stgcnpp_rtmpose_generator/stgcnpp_rtmpose_sahi_rtsp.hpp"

#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

StgcnppRtmposeSahiRtspGenerator::StgcnppRtmposeSahiRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sgie,
    nlohmann::json stgcnpp,
    nlohmann::json sahi,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)),
      sgie(std::move(sgie)),
      stgcnpp(std::move(stgcnpp)) {
    generator = "StgcnppRtmposeSahiRTSPGenerator";
}

void StgcnppRtmposeSahiRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}

void StgcnppRtmposeSahiRtspGenerator::initParams() {
    SahiRtspGenerator::initParams();
    pose.emplace(sgie, pipelineName, static_cast<int>(streams.size()));
    pose->init();
    model.emplace(stgcnpp, pipelineName, static_cast<int>(streams.size()));
    model->init(pgie.at("interval").get<int>());
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["sgie"] = pose->sgieParams();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["stgcnpp"] = model->params();
    files.insert_or_assign(TopdownPose::SGIE_CONFIG_NAME, ConfigFile(pose->sgieConfig));
    files.insert_or_assign(TopdownPose::RTMPOSE_PREPROCESS_CONFIG_NAME, ConfigFile(pose->preprocessIni));
    files.insert_or_assign(StgcnppYoloPose::CONFIG_NAME, ConfigFile(model->config));
    files.insert_or_assign(StgcnppYoloPose::PREPROCESS_CONFIG_NAME, ConfigFile(model->preprocessIni));
    files.insert_or_assign(StgcnppYoloPose::META_NAME, ConfigFile(model->metaText));
}

void StgcnppRtmposeSahiRtspGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    SahiRtspGenerator::applySavePaths(pipelineDocument, configSaveDir);
    pose->applySavePaths(pipelineDocument, configSaveDir);
    model->applySavePaths(pipelineDocument, configSaveDir);
}

void StgcnppRtmposeSahiRtspGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(item.value().at("url").get<std::string>(), true));
        index += 1;
    }
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(muxBatchSize, width, height, true, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId));
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
    if (enableNvtracker) {
        if (drawer.has_value()) {
            appendNode("nvbboxsnapshot", "nvbboxsnapshot", addNvbboxsnapshot());
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
    appendNode("nvstgcnppactionlabel", "nvstgcnppactionlabel", addNvstgcnppactionlabel(model->uniqueId));
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME, gpuId));
    appendNode("tee", "tee_msg", addTee());
    appendNode("queue", "queue_msg", addQueue());
    appendNode("nvmsgconv", "nvmsgconv", addNvmsgconv(MSGCONV_CONFIG_NAME, pipeline::PAYLOAD_DEEPSTREAM_MINIMAL));
    appendNode(
        "nvmsgbroker",
        "nvmsgbroker",
        addNvmsgbroker(subelement::KAFKA_PROTO_LIB, subelement::KAFKA_CONN_STR, kafkaTopic, KAFKA_CONFIG_NAME, false, false));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < muxBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode(
            "nvstgcnpplogger",
            "nvstgcnpplogger" + suffix,
            addNvstgcnpplogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
        appendNode("fakesink", "fakesink" + suffix, addFakesink(false, false));
    }
}

void StgcnppRtmposeSahiRtspGenerator::link() {
    YAML::Node edges(YAML::NodeType::Map);
    for (int index = 0; index < muxBatchSize; ++index) {
        edges["nvurisrcbin" + std::to_string(index)] = "nvstreammux";
    }
    edges["nvstreammux"] = "nvsahipreprocess";
    edges["nvsahipreprocess"] = "pgie";
    edges["pgie"] = "queue_sahi";
    edges["queue_sahi"] = sahiPostprocess;
    std::string inferenceTail = sahiPostprocess;
    if (enableNvtracker) {
        if (drawer.has_value()) {
            edges[inferenceTail] = "nvbboxsnapshot";
            edges["nvbboxsnapshot"] = "nvtracker";
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
    edges["sgie1"] = "nvstgcnppactionlabel";
    edges["nvstgcnppactionlabel"] = "nvdsanalytics";
    edges["nvdsanalytics"] = "tee_msg";
    edges["tee_msg"].push_back("nvstreamdemux");
    edges["tee_msg"].push_back("queue_msg");
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    for (int index = 0; index < muxBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "nvstgcnpplogger" + suffix;
        edges["nvstgcnpplogger" + suffix] = "fakesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
