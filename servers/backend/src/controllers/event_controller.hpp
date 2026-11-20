#pragma once

#include "constants/event_const.hpp"
#include "logger/event_logger.hpp"
#include "services/event_service.hpp"

#include <drogon/HttpController.h>

#include <functional>
#include <string>

class EventController : public drogon::HttpController<EventController> {
 public:
  EventController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(EventController::list, event::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(EventController::read, event::PATH_READ, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(EventController::unread, event::PATH_UNREAD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(EventController::remove, event::PATH_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void read(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& event_id);
  void unread(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& event_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& event_id);

 private:
  void setRead(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& eventId,
               bool read,
               const char* ok,
               const char* error);

  EventService service;
  EventLogger logger;
};
