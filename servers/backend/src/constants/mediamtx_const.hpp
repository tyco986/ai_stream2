#pragma once

#include <array>

namespace mediamtx_backend {

inline const char* REPLACE_PATH = "/v3/config/paths/replace/";
inline const char* DELETE_PATH = "/v3/config/paths/delete/";
inline const char* GET_PATH = "/v3/config/paths/get/";
inline const char* KEY_SOURCE = "source";
inline const char* KEY_RECORD = "record";
inline const char* SOURCE_PUBLISHER = "publisher";
inline constexpr std::array<const char*, 3> PUBLISHER_HOSTS{"puskas_mediamtx", "127.0.0.1", "localhost"};
inline const double REQUEST_TIMEOUT_SECONDS = 5;

}
