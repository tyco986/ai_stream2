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
    std::filesystem::create_directories(output.parent_path());
}

void VisEventVideoGenerator::initParams() {
    EventVideoGenerator::initParams();
    std::get<YAML::Node>(files.at(PARAMS_CONFIG_NAME))["output"] = output.string();
}

void VisEventVideoGenerator::appendEncode(int gpuId) {
    appendNode("nvstreamdemux", "nvstreamdemux", addNvstreamdemux());
    for (int streamIndex = 0; streamIndex < static_cast<int>(videos.size()); ++streamIndex) {
        const std::string suffix = std::to_string(streamIndex);
        const std::filesystem::path source(videos.at(streamIndex).get<std::string>());
        const std::filesystem::path sink = output / (suffix + "_" + source.stem().string() + ".mp4");
        appendNode("queue", "queue_demux" + suffix, addQueue());
        appendNode(
            "nvv4l2h264enc",
            "nvv4l2h264enc" + suffix,
            addNvv4l2h264enc(pipeline::H264_BITRATE, videoFps[streamIndex], pipeline::H264_PRESET_ID, gpuId));
        appendNode("h264parse", "h264parse" + suffix, addH264parse());
        appendNode("mp4mux", "mp4mux" + suffix, addMp4mux());
        appendNode("filesink", "filesink" + suffix, addFilesink(sink, false, false));
    }
}

void VisEventVideoGenerator::linkEncode(YAML::Node edges) {
    edges["presencelogger"] = "nvstreamdemux";
    for (int index = 0; index < static_cast<int>(videos.size()); ++index) {
        const std::string suffix = std::to_string(index);
        edges["queue_demux" + suffix] = "nvv4l2h264enc" + suffix;
        edges["nvv4l2h264enc" + suffix] = "h264parse" + suffix;
        edges["h264parse" + suffix] = "mp4mux" + suffix;
        edges["mp4mux" + suffix] = "filesink" + suffix;
    }
}
