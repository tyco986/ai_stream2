#pragma once

#include <json/json.h>

#include <string>

class KafkaLogger {
 public:
  static void write(const std::string& topic, const Json::Value& payload);

 private:
  static std::string timestamp();
};
