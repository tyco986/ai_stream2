#pragma once

#include <json/json.h>

class Kafka {
 public:
  void run();

 private:
  void dispatch(const Json::Value& message);
};
