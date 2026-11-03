#pragma once

namespace device {

inline const char* KEY_NAME = "name";
inline const char* KEY_MEMORY_TOTAL = "memory_total";
inline const char* KEY_MEMORY_USED = "memory_used";
inline const char* KEY_MEMORY_UTILIZATION = "memory_utilization";
inline const char* KEY_GPU_UTILIZATION = "gpu_utilization";

inline const char* PATH_LIST = "/backend/device";
inline const char* MSG_LIST_OK = "device_list_ok";
inline const char* MSG_LIST_ERROR = "device_list_error";

inline const unsigned int NAME_BUFFER_SIZE = 96;

}
