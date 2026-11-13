#include "controllers/event_ws.hpp"

#include "services/event_hub.hpp"

void EventWs::handleNewMessage(const drogon::WebSocketConnectionPtr&,
                               std::string&&,
                               const drogon::WebSocketMessageType&) {}

void EventWs::handleNewConnection(const drogon::HttpRequestPtr&,
                                  const drogon::WebSocketConnectionPtr& conn) {
  EventHub::shared().join(conn);
}

void EventWs::handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) {
  EventHub::shared().leave(conn);
}
