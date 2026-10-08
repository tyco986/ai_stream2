#pragma once

#include "constants/log.hpp"

#include <drogon/HttpController.h>

class LogController : public drogon::HttpController<LogController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(LogController::get, log::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(LogController::enable, log::PATH_ENABLE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void get(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void enable(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  bool dateValid(const std::string& date);
};
