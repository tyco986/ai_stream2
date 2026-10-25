#pragma once

namespace rtsp {

inline constexpr const char* RTSP_PROBE_PATH = "/rtsp/probe";
inline constexpr const char* RTSP_PUBLISH_PATH = "/rtsp/publish";
inline constexpr int FFPROBE_TIMEOUT = 5;
inline constexpr const char* FFPROBE_BINARY = "ffprobe";
inline constexpr const char* FFMPEG_BINARY = "ffmpeg";
inline constexpr const char* KEY_PATHS = "paths";
inline constexpr const char* KEY_URLS = "urls";
inline constexpr const char* KEY_NAME = "name";
inline constexpr const char* KEY_PID = "pid";
inline constexpr const char* KEY_CREATED_AT = "created_at";
inline constexpr const char* KEY_URL_DOCKER = "url_docker";
inline constexpr const char* KEY_URL_HOST = "url_host";
inline constexpr const char* KEY_WIDTH = "width";
inline constexpr const char* KEY_HEIGHT = "height";
inline constexpr const char* KEY_FPS = "fps";
inline constexpr const char* KEY_CODEC = "codec";
inline constexpr const char* KEY_BITRATE = "bitrate";
inline constexpr const char* KEY_LOOP = "loop";
inline constexpr const char* KEY_DURATION = "duration";
inline constexpr const char* KEY_STATUS = "status";
inline constexpr const char* KEY_PROBED_AT = "probed_at";
inline constexpr const char* KEY_READY = "ready";
inline constexpr const char* STATUS_OFFLINE = "offline";
inline constexpr const char* MEDIAMTX_HOST = "puskas_mediamtx";
inline constexpr int MEDIAMTX_API_PORT = 9997;
inline constexpr const char* MEDIAMTX_PATH_GET = "/v3/paths/get/";
inline constexpr const char* MEDIAMTX_INFO_PATH = "/v3/info";
inline constexpr int MEDIAMTX_HEALTH_TIMEOUT_SECONDS = 3;
inline constexpr int PUBLISH_POLL_SECONDS = 1;
inline constexpr int PUBLISH_TIMEOUT_SECONDS = 5;
inline constexpr const char* URL_DOCKER_PREFIX = "rtsp://puskas_mediamtx:8554/";
inline constexpr const char* URL_HOST_PREFIX = "rtsp://127.0.0.1:8554/";
inline constexpr const char* MSG_PUBLISH_ERROR = "Failed to publish rtsp";
inline constexpr const char* MSG_MEDIAMTX_UNREACHABLE = "MediaMTX unreachable";

}
