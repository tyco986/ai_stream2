#include "topdown_pose_generator/topdown_pose_rtsp.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"
#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

TopdownPoseRtspGenerator::TopdownPoseRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)),
      sgie(std::move(sgie)) {
    generator = "TopdownPoseRTSPGenerator";
}

void TopdownPoseRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}

void TopdownPoseRtspGenerator::initParams() {
    BaseRtspGenerator::initParams();
    pose.emplace(sgie, static_cast<int>(streams.size()));
    pose->init();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["sgie"] = pose->sgieParams();
    files.insert_or_assign(TopdownPose::SGIE_CONFIG_NAME, ConfigFile(pose->sgieConfig));
    files.insert_or_assign(TopdownPose::RTMPOSE_PREPROCESS_CONFIG_NAME, ConfigFile(pose->preprocessIni));
}

void TopdownPoseRtspGenerator::applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) {
    BaseRtspGenerator::applySavePaths(pipelineDocument, configSaveDir);
    pose->applySavePaths(pipelineDocument, configSaveDir);
}

void TopdownPoseRtspGenerator::add() {
    int index = 0;
    const int gpuId = pgieGenerator->config["property"]["gpu-id"].as<int>();
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        appendNode("nvurisrcbin", "nvurisrcbin" + std::to_string(index), addNvurisrcbin(item.value().at("url").get<std::string>(), true));
        index += 1;
    }
    appendNode(
        "nvstreammux",
        "nvstreammux",
        addNvstreammux(static_cast<int>(streams.size()), width, height, true, false, pipeline::BATCHED_PUSH_TIMEOUT_US, gpuId));
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
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME, gpuId));
    appendNode("tee", "tee_msg", addTee());
    appendNode("queue", "queue_msg", addQueue());
    appendNode("nvmsgconv", "nvmsgconv", addNvmsgconv(MSGCONV_CONFIG_NAME, pipeline::PAYLOAD_DEEPSTREAM_MINIMAL));
    appendNode(
        "nvmsgbroker",
        "nvmsgbroker",
        addNvmsgbroker(subelement::KAFKA_PROTO_LIB, subelement::KAFKA_CONN_STR, kafkaTopic, KAFKA_CONFIG_NAME, false, false));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < runtimeBatchSize; ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode(
            "detlogger",
            "detlogger" + suffix,
            addDetlogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
        appendNode("fakesink", "fakesink" + suffix, addFakesink(false, false));
    }
}

void TopdownPoseRtspGenerator::link() {
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
    edges["nvrtmposepostprocess"] = "nvdsanalytics";
    edges["nvdsanalytics"] = "tee_msg";
    edges["tee_msg"].push_back("nvstreamdemux");
    edges["tee_msg"].push_back("queue_msg");
    edges["queue_msg"] = "nvmsgconv";
    edges["nvmsgconv"] = "nvmsgbroker";
    for (int index = 0; index < runtimeBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "detlogger" + suffix;
        edges["detlogger" + suffix] = "fakesink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
