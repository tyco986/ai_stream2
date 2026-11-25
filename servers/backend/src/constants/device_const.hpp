#pragma once

namespace device {

inline const char* HUB_BASE_URL = "http://ai_stream2_beszel:8090";
inline const char* HUB_IDENTITY = "admin@example.com";
inline const char* HUB_PASSWORD = "88888888";
inline const char* AUTH_PATH = "/api/collections/users/auth-with-password";
inline const char* RECORDS_PREFIX = "/api/collections/";
inline const char* RECORDS_SUFFIX = "/records";
inline const char* COLLECTION_SYSTEMS = "systems";
inline const char* COLLECTION_DETAILS = "system_details";
inline const char* COLLECTION_STATS = "system_stats";
inline const char* STATS_QUERY = "&sort=-created&filter=type%3D%221m%22";
inline const char* AUTH_PREFIX = "Bearer ";
inline const int PAGE_SIZE = 200;
inline const double REQUEST_TIMEOUT_SECONDS = 10;
inline const unsigned int BYTES_PER_MB = 1024 * 1024;
inline const unsigned int SECONDS_PER_DAY = 86400;
inline const unsigned int SECONDS_PER_HOUR = 3600;
inline const unsigned int SECONDS_PER_MINUTE = 60;

inline const char* KEY_SYSTEM = "system";
inline const char* KEY_CPU = "cpu";
inline const char* KEY_MEMORY = "memory";
inline const char* KEY_GPU = "gpu";
inline const char* KEY_NAME = "name";
inline const char* KEY_HOST = "host";
inline const char* KEY_STATUS = "status";
inline const char* KEY_HOSTNAME = "hostname";
inline const char* KEY_OS_NAME = "os_name";
inline const char* KEY_KERNEL = "kernel";
inline const char* KEY_ARCH = "arch";
inline const char* KEY_UPTIME = "uptime";
inline const char* KEY_DAYS = "days";
inline const char* KEY_HOURS = "hours";
inline const char* KEY_MINUTES = "minutes";
inline const char* KEY_MODEL = "model";
inline const char* KEY_CORES = "cores";
inline const char* KEY_THREADS = "threads";
inline const char* KEY_UTILIZATION = "utilization";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_USED = "used";
inline const char* KEY_MEMORY_TOTAL = "memory_total";
inline const char* KEY_MEMORY_USED = "memory_used";

inline const char* PATH_LIST = "/backend/nodes";
inline const char* MSG_LIST_OK = "node_list_ok";
inline const char* MSG_LIST_ERROR = "node_list_error";

}
