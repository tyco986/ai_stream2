#pragma once

#include "constants/server_const.hpp"
#include "logger/server_logger.hpp"
#include "services/server_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

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
  drogon::Task<drogon::HttpResponsePtr> probe(drogon::HttpRequestPtr request);

 private:
  ServerService service;
  ServerLogger logger;
};
