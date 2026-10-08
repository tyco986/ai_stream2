#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string_view>

namespace constants {

constexpr std::string_view API_PREFIX = "/puskas/ffmpeg";
constexpr std::string_view MEDIAMTX_HOST = "puskas_mediamtx";
constexpr int MEDIAMTX_PORT = 8554;

constexpr std::string_view LOGGER_NAME = "ffmpeg_api";
constexpr std::string_view LOG_FILE_NAME = "app.log";
constexpr std::size_t LOG_MAX_BYTES = 1024 * 1024;

constexpr std::array<std::string_view, 4> FFMPEG_BASE{
    "ffmpeg",
    "-hide_banner",
    "-loglevel",
    "warning",
};

constexpr std::array<std::string_view, 9> FFPROBE_BASE{
    "ffprobe",
    "-v",
    "error",
    "-rtsp_transport",
    "tcp",
    "-print_format",
    "json",
    "-show_format",
    "-show_streams",
};

constexpr std::chrono::milliseconds PUBLISHER_START_TIMEOUT{1000};
constexpr std::chrono::milliseconds PUBLISHER_STOP_TIMEOUT{5000};

constexpr std::string_view RECORDINGS_ROOT = "/root/recordings";
constexpr std::string_view INPUT_ROOT = "/root/tmp";
constexpr std::string_view CAPTURE_OUTPUT_ROOT = "/root/outputs/ffmpeg/capture";
constexpr std::string_view EXTRACT_OUTPUT_ROOT = "/root/outputs/ffmpeg/extract";
constexpr std::string_view NOB_OUTPUT_ROOT = "/root/outputs/nob";
constexpr std::string_view LOG_ROOT = "/root/logs/ffmpeg";

constexpr std::string_view TIMESTAMP_PATTERN = R"(^\d{2}:\d{2}:\d{2}(\.\d{1,3})?$)";

}
