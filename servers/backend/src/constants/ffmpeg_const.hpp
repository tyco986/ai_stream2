#pragma once

namespace ffmpeg_backend {

inline const char* RTSP_PROBE_PATH = "/rtsp/probe";
inline const char* RTSP_PUBLISH_PATH = "/rtsp/publish";
inline const char* KEY_URL = "url";
inline const char* KEY_URLS = "urls";
inline const char* KEY_PATHS = "paths";
inline const char* KEY_NAMES = "names";
inline const char* KEY_URL_DOCKER = "url_docker";
inline const char* KEY_STATUS = "status";
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_FPS = "fps";
inline const char* KEY_PROBED_AT = "probed_at";
inline const double REQUEST_TIMEOUT_SECONDS = 10;
inline const double PUBLISH_TIMEOUT_SECONDS = 20;

}
