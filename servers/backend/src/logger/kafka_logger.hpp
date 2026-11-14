#pragma once

#include <json/json.h>
#include <spdlog/logger.h>

#include <memory>
#include <mutex>
#include <string>

class KafkaLogger {
 public:
  static void write(const std::string& topic, const Json::Value& payload);

 private:
  static std::string timestamp();

  static std::mutex logMutex;
  static std::shared_ptr<spdlog::logger> logger;
};
