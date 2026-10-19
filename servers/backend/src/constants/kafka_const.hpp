#pragma once

namespace kafka {

inline const char* BROKERS = "puskas_kafka:9092";
inline const char* GROUP = "backend";
inline const char* PROJECT = "ai_stream2";
inline const char* TOPIC_PATTERN = "^ai_stream2_.*";
inline const int POLL_MS = 1000;

}
