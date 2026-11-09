#include "base_generator/vis_event_video.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

VisEventVideoGenerator::VisEventVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : EventVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(analyzer),
          std::move(pgie),
          std::move(eventCoder),
          std::move(tracker),
          std::move(logger),
          std::move(drawer),
          std::move(capturer)),
      output(std::move(output)) {
}

void VisEventVideoGenerator::initVideos() {
    EventVideoGenerator::initVideos();
    std::filesystem::create_directories(output);
}

void VisEventVideoGenerator::initParams() {
    EventVideoGenerator::initParams();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["output"] = output.string();
}

void VisEventVideoGenerator::appendAfterOsd(int index, int gpuId) {
    const std::string suffix = std::to_string(index);
    const std::filesystem::path source(videos.at(index).get<std::string>());
    const std::filesystem::path sink = output / source.filename();
    appendNode("tee", "tee_vis" + suffix, addTee());
    appendNode("queue", "queue_vis" + suffix, addQueue());
    appendNode("nvvideoconvert", "nvvideoconvert_vis" + suffix, addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_vis" + suffix, addCapsfilter("video/x-raw(memory:NVMM), format=RGB"));
    appendNode("viscapturer", "viscapturer" + suffix, addViscapturer(capturerCodes()));
    appendNode("fakesink", "fakesink_vis" + suffix, addFakesink(false, false));
    appendNode("queue", "queue_enc" + suffix, addQueue());
    appendNode(
        "presencelogger",
        "presencelogger" + suffix,
        addPresencelogger(logger.value("interval", 0)));
    appendNode("nvvideoconvert", "nvvideoconvert_enc" + suffix, addNvvideoconvert(gpuId));
    appendNode("capsfilter", "capsfilter_enc" + suffix, addCapsfilter(pipeline::H264_CAPS));
    appendNode(
        "nvv4l2h264enc",
        "nvv4l2h264enc" + suffix,
        addNvv4l2h264enc(pipeline::H264_BITRATE, videoFps[index], pipeline::H264_PRESET_ID, gpuId));
    appendNode("h264parse", "h264parse" + suffix, addH264parse());
    appendNode("mp4mux", "mp4mux" + suffix, addMp4mux());
    appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
}

void VisEventVideoGenerator::linkAfterOsd(YAML::Node edges, int index) {
    const std::string suffix = std::to_string(index);
    edges["nvosdbin" + suffix] = "tee_vis" + suffix;
    edges["tee_vis" + suffix].push_back("queue_vis" + suffix);
    edges["tee_vis" + suffix].push_back("queue_enc" + suffix);
    edges["queue_vis" + suffix] = "nvvideoconvert_vis" + suffix;
    edges["nvvideoconvert_vis" + suffix] = "capsfilter_vis" + suffix;
    edges["capsfilter_vis" + suffix] = "viscapturer" + suffix;
    edges["viscapturer" + suffix] = "fakesink_vis" + suffix;
    edges["queue_enc" + suffix] = "presencelogger" + suffix;
    edges["presencelogger" + suffix] = "nvvideoconvert_enc" + suffix;
    edges["nvvideoconvert_enc" + suffix] = "capsfilter_enc" + suffix;
    edges["capsfilter_enc" + suffix] = "nvv4l2h264enc" + suffix;
    edges["nvv4l2h264enc" + suffix] = "h264parse" + suffix;
    edges["h264parse" + suffix] = "mp4mux" + suffix;
    edges["mp4mux" + suffix] = "filesink" + suffix;
}
