#pragma once

#include "constants/event_const.hpp"
#include "log/event_logger.hpp"
#include "services/event_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>
#include <functional>
#include <string>

class EventController : public drogon::HttpController<EventController> {
 public:
  EventController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(EventController::list, event::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(EventController::setStatus, event::PATH_STATUS, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void setStatus(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 const std::string& event_id);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  EventService service;
  EventLogger logger;
};
