#pragma once

#include "constants/service_log.hpp"
#include "logger/log_logger.hpp"

#include <drogon/HttpController.h>

class LogController : public drogon::HttpController<LogController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(LogController::get, service_log::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(LogController::remove, service_log::PATH_LIST, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(LogController::enable, service_log::PATH_ENABLE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(LogController::disable, service_log::PATH_DISABLE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(LogController::status, service_log::PATH_STATUS, drogon::Get, "AuthFilter");
  METHOD_LIST_END

  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void enable(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void disable(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void status(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  LogLogger logger;
};
