#pragma once

#include "constants/user_const.hpp"
#include "logger/user_logger.hpp"
#include "services/user_service.hpp"

#include <drogon/HttpController.h>

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
  UserService service;
  UserLogger logger;
};
