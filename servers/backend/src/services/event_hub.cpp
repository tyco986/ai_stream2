#include "services/event_hub.hpp"

#include "constants/response_const.hpp"

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
