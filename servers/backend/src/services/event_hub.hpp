#pragma once

#include <drogon/WebSocketConnection.h>
#include <json/json.h>
#include <trantor/net/EventLoop.h>

#include <mutex>
#include <vector>

class EventHub {
 public:
  static EventHub& shared();
  void join(const drogon::WebSocketConnectionPtr& conn);
  void leave(const drogon::WebSocketConnectionPtr& conn);
  void refresh();
  void banner(const char* action, const Json::Value& event);

 private:
  void schedule();
  void emit();
  void publish(const Json::Value& row);

  std::mutex mutex;
  std::vector<drogon::WebSocketConnectionPtr> connections;
  trantor::TimerId timer;
  bool armed = false;
};
