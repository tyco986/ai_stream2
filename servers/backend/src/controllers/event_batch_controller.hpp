#pragma once

#include "constants/event_const.hpp"
#include "logger/event_logger.hpp"
#include "services/event_service.hpp"

#include <drogon/HttpController.h>

#include <functional>

class EventBatchController : public drogon::HttpController<EventBatchController> {
 public:
  EventBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(EventBatchController::batchRead, event::PATH_BATCH_READ, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(EventBatchController::batchUnread, event::PATH_BATCH_UNREAD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(EventBatchController::batchRemove, event::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void batchRead(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchUnread(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  void apply(const drogon::HttpRequestPtr& request,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
             bool remove,
             bool read,
             const char* message);

  EventService service;
  EventLogger logger;
};
