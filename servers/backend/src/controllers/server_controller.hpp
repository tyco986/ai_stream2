#pragma once

#include "constants/server_const.hpp"
#include "log/server_logger.hpp"
#include "services/server_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <functional>

class ServerController : public drogon::HttpController<ServerController> {
 public:
  ServerController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ServerController::get, server::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(ServerController::probe, server::PATH_PROBE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  drogon::Task<drogon::HttpResponsePtr> probe(const drogon::HttpRequestPtr& request);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  ServerService service;
  ServerLogger logger;
};
