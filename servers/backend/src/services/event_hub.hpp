#pragma once

#include <drogon/WebSocketConnection.h>
#include <json/json.h>

#include <mutex>
#include <vector>

class EventHub {
 public:
  static EventHub& shared();
  void join(const drogon::WebSocketConnectionPtr& conn);
  void leave(const drogon::WebSocketConnectionPtr& conn);
  void publish(const Json::Value& row);

 private:
  std::mutex mutex;
  std::vector<drogon::WebSocketConnectionPtr> connections;
};
