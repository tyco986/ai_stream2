#include "services/event_hub.hpp"

#include "constants/event_const.hpp"
#include "constants/response_const.hpp"

#include <drogon/drogon.h>

#include <functional>

EventHub& EventHub::shared() {
  static EventHub hub;
  return hub;
}

void EventHub::join(const drogon::WebSocketConnectionPtr& conn) {
  std::lock_guard<std::mutex> lock(mutex);
  connections.push_back(conn);
}

void EventHub::leave(const drogon::WebSocketConnectionPtr& conn) {
  std::lock_guard<std::mutex> lock(mutex);
  std::erase(connections, conn);
}

void EventHub::refresh() {
  drogon::app().getLoop()->runInLoop(std::bind(&EventHub::schedule, this));
}

void EventHub::banner(const char* action, const Json::Value& event) {
  drogon::app().getLoop()->runInLoop(std::bind(&EventHub::publish, this, event::BANNER(action, event)));
}

void EventHub::schedule() {
  auto* loop = drogon::app().getLoop();
  if (armed) {
    loop->invalidateTimer(timer);
  }
  armed = true;
  timer = loop->runAfter(event::REFRESH_QUIET_SECONDS, std::bind(&EventHub::emit, this));
}

void EventHub::emit() {
  armed = false;
  Json::Value signal;
  signal[event::KEY_REFRESH] = true;
  publish(signal);
}

void EventHub::publish(const Json::Value& row) {
  Json::StreamWriterBuilder builder;
  builder[http::JSON_INDENTATION] = "";
  builder[http::JSON_PRECISION_TYPE] = http::JSON_PRECISION_DECIMAL;
  builder[http::JSON_PRECISION] = http::JSON_PRECISION_DIGITS;
  const std::string text = Json::writeString(builder, row);
  std::lock_guard<std::mutex> lock(mutex);
  for (const auto& conn : connections) {
    if (conn && conn->connected()) {
      conn->send(text);
    }
  }
}
