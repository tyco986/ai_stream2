#include "base_generator/vis_event_sahi_rtsp.hpp"

#include "constants/pipeline_const.hpp"

static std::string streamUrlAt(const nlohmann::json& streams, int index) {
    int cursor = 0;
    std::string url;
    for (auto item = streams.begin(); item != streams.end(); ++item) {
        if (cursor == index) {
            url = item.value().at("url").get<std::string>();
        }
        cursor += 1;
    }
    return url;
}

void VisEventSahiRtspGenerator::appendAfterOsd(int index, int gpuId) {
    const std::string suffix = std::to_string(index);
    appendNode("tee", "tee_vis" + suffix, addTee());
    appendNode("queue", "queue_vis" + suffix, addQueue());
    appendNode("nvvideoconvert", "nvvideoconvert_vis" + suffix, addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_vis" + suffix, addCapsfilter("video/x-raw(memory:NVMM), format=RGB"));
    appendNode("viscapturer", "viscapturer" + suffix, addViscapturer(captureOutputDir(), capturerCodes()));
    appendNode("fakesink", "fakesink_vis" + suffix, addFakesink(false, false));
    appendNode("queue", "queue_enc" + suffix, addQueue());
    appendNode(
        "presencelogger",
        "presencelogger" + suffix,
        addPresencelogger(std::string(pipeline::DEEPSTREAM_LOG_ROOT) + "/" + pipelineName, logger.value("interval", 0)));
    appendNode(
        "nvv4l2h264enc",
        "nvv4l2h264enc" + suffix,
        addNvv4l2h264enc(pipeline::H264_BITRATE, fps, pipeline::H264_PRESET_ID, gpuId));
    appendNode("h264parse", "h264parse" + suffix, addH264parse());
    appendNode(
        "rtspclientsink",
        "rtspclientsink" + suffix,
        addRtspclientsink(visualizedSinkUri(streamUrlAt(streams, index)), false, false));
}

void VisEventSahiRtspGenerator::linkAfterOsd(YAML::Node edges, int index) {
    const std::string suffix = std::to_string(index);
    edges["nvosdbin" + suffix] = "tee_vis" + suffix;
    edges["tee_vis" + suffix].push_back("queue_vis" + suffix);
    edges["tee_vis" + suffix].push_back("queue_enc" + suffix);
    edges["queue_vis" + suffix] = "nvvideoconvert_vis" + suffix;
    edges["nvvideoconvert_vis" + suffix] = "capsfilter_vis" + suffix;
    edges["capsfilter_vis" + suffix] = "viscapturer" + suffix;
    edges["viscapturer" + suffix] = "fakesink_vis" + suffix;
    edges["queue_enc" + suffix] = "presencelogger" + suffix;
    edges["presencelogger" + suffix] = "nvv4l2h264enc" + suffix;
    edges["nvv4l2h264enc" + suffix] = "h264parse" + suffix;
    edges["h264parse" + suffix] = "rtspclientsink" + suffix;
}
