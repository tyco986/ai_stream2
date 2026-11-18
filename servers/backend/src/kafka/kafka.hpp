#pragma once

#include "services/event_service.hpp"

#include <json/json.h>
#include <librdkafka/rdkafkacpp.h>

#include <cstdint>
#include <mutex>
#include <set>
#include <string>

class Kafka {
 public:
  static Kafka& shared();
  void watch(const std::string& name);
  void run();

 private:
  Kafka();
  void dispatch(const Json::Value& message, const std::string& topic);
  void apply(RdKafka::KafkaConsumer& consumer);

  EventService service;
  std::uint64_t seen;
  std::mutex mutex;
  std::uint64_t revision;
  std::set<std::string> names;
};
