#pragma once

#include "services/event_service.hpp"

#include <json/json.h>

class Kafka {
 public:
  Kafka();
  void run();

 private:
  void dispatch(const Json::Value& message);

  EventService service;
};
