#pragma once

namespace redis {

inline const char* HOST = "ai_stream2_redis";
inline const int PORT = 6379;
inline const char* GROUP = "backend";
inline const char* CONSUMER = "backend";
inline const char* TOPIC_PREFIX = "ai_stream2_";
inline const char* PAYLOAD_KEY = "metadata";
inline const char* STREAM_START = "0";
inline const char* STREAM_NEW = ">";
inline const char* BUSYGROUP = "BUSYGROUP";
inline const char* KEY_TS = "ts";
inline const char* KEY_PAD = "pad";
inline const char* KEY_EVENT = "event";
inline const char* KEY_NUM = "num";
inline const char* KEY_OBJECT = "object";
inline const int BLOCK_MS = 1000;
inline const int READ_COUNT = 32;

inline const char* LOG_DIR = "redis";
inline const char* LOG_NAME = "redis";
inline const char* KEY_TOPIC = "topic";
inline const char* KEY_PAYLOAD = "payload";

}
