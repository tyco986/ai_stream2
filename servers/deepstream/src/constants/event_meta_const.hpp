#pragma once

namespace eventmeta {

inline constexpr char ABSENT = '0';
inline constexpr char ALERT = '1';
inline constexpr char TRANSIT = '2';
inline constexpr const char* LEVEL_ALERT = "alert";
inline constexpr const char* LEVEL_TRANSIT = "transit";
inline constexpr const char* LEVEL_ABSENT = "absent";
inline constexpr int SLOT_COUNT = 8;
inline constexpr int NAME_LEN = 64;

}
