#pragma once

#include "services/event_service.hpp"

#include <hiredis/hiredis.h>

#include <mutex>
#include <set>
#include <string>
#include <vector>

class Redis {
 public:
  static Redis& shared();
  void watch(const std::string& name);
  void run();

 private:
  Redis();
  std::vector<std::string> streams();
  void connect();
  redisReply* command(const std::vector<std::string>& args);
  void ensureGroups(const std::vector<std::string>& keys);
  bool read(const std::vector<std::string>& keys, const char* id, bool block);
  void collect(redisReply& reply);
  void handle(const std::string& stream, const std::string& id, const std::string& payload);
  void dispatch(const Json::Value& message, const std::string& stream);
  void acknowledge(const std::string& stream, const std::string& id);
  void close();

  EventService service;
  std::mutex mutex;
  std::set<std::string> names;
  std::set<std::string> grouped;
  redisContext* context = nullptr;
};
