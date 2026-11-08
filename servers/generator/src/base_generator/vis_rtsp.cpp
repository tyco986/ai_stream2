#include "base_generator/vis_rtsp.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/subelement_const.hpp"

void VisRtspGenerator::add() {
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
    appendNode("nvdsanalytics", "nvdsanalytics", addNvdsanalytics(ANALYTICS_CONFIG_NAME));
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    index = 0;
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        const std::string suffix = std::to_string(index);
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode("nvvideoconvert", "nvvideoconvert" + suffix, addNvvideoconvert(gpuId));
        if (drawer.has_value()) {
            const std::string drawerName = nvdetDrawerElement() + suffix;
            appendNode(nvdetDrawerElement(), drawerName, nvdetDrawerProperties(*drawer));
        }
        appendNode("nvosdbin", "nvosdbin" + suffix, addNvosdbin(gpuId, true, true));
        appendNode("queue", "queue_enc" + suffix, addQueue());
        appendNode(
            "detlogger",
            "detlogger" + suffix,
            addDetlogger(logger.value("interval", 0)));
        appendNode(
            "nvv4l2h264enc",
            "nvv4l2h264enc" + suffix,
            addNvv4l2h264enc(pipeline::H264_BITRATE, fps, pipeline::H264_PRESET_ID, gpuId));
        appendNode("h264parse", "h264parse" + suffix, addH264parse());
        appendNode(
            "rtspclientsink",
            "rtspclientsink" + suffix,
            addRtspclientsink(visualizedSinkUri(item.value().at("url").get<std::string>()), false, false));
        index += 1;
    }
}

void VisRtspGenerator::link() {
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
    edges[inferenceTail] = "nvdsanalytics";
    edges["nvdsanalytics"] = "nvstreamdemux";
    for (int index = 0; index < runtimeBatchSize; ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "nvvideoconvert" + suffix;
        const std::string drawerName = nvdetDrawerElement() + suffix;
        const std::string osdPrev = drawer.has_value() ? drawerName : "nvosdbin" + suffix;
        edges["nvvideoconvert" + suffix] = osdPrev;
        if (drawer.has_value()) {
            edges[drawerName] = "nvosdbin" + suffix;
        }
        edges["nvosdbin" + suffix] = "queue_enc" + suffix;
        edges["queue_enc" + suffix] = "detlogger" + suffix;
        edges["detlogger" + suffix] = "nvv4l2h264enc" + suffix;
        edges["nvv4l2h264enc" + suffix] = "h264parse" + suffix;
        edges["h264parse" + suffix] = "rtspclientsink" + suffix;
    }
    pipeline["deepstream"]["edges"] = edges;
}
