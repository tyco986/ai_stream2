#pragma once

#include "constants/user_const.hpp"
#include "log/user_logger.hpp"
#include "services/user_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>
#include <functional>

class UserBatchController : public drogon::HttpController<UserBatchController> {
 public:
  UserBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(UserBatchController::batchRemove, user::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  UserService service;
  UserLogger logger;
};
