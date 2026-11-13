#pragma once

#include <drogon/WebSocketController.h>

class EventWs : public drogon::WebSocketController<EventWs> {
 public:
  void handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
                        std::string&& message,
                        const drogon::WebSocketMessageType& type) override;
  void handleNewConnection(const drogon::HttpRequestPtr& request,
                           const drogon::WebSocketConnectionPtr& conn) override;
  void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override;

  WS_PATH_LIST_BEGIN
  WS_PATH_ADD("/backend/events/ws", "AuthFilter");
  WS_PATH_LIST_END
};
