#pragma once

namespace kafka {

inline const char* BROKERS = "ai_stream2_kafka:9092";
inline const char* GROUP = "backend";
inline const char* PROJECT = "ai_stream2";
inline const char* TOPIC_PATTERN = "^ai_stream2_.*";
inline const char* TOPIC_PREFIX = "ai_stream2_";
inline const char* KEY_TS = "ts";
inline const char* KEY_PAD = "pad";
inline const char* KEY_EVENT = "event";
inline const char* KEY_NUM = "num";
inline const char* KEY_OBJECT = "object";
inline const int POLL_MS = 1000;

}
