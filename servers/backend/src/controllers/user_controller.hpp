#pragma once

#include "constants/user_const.hpp"
#include "log/user_logger.hpp"
#include "services/user_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>

class UserController : public drogon::HttpController<UserController> {
 public:
  UserController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(UserController::login, user::PATH_USER_LOGIN, drogon::Post);
  ADD_METHOD_TO(UserController::list, user::PATH_USER_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserController::map, user::PATH_USER_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserController::create, user::PATH_USER_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(UserController::get, user::PATH_USER_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserController::update, user::PATH_USER_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(UserController::remove, user::PATH_USER_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void login(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& user_id);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& user_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& user_id);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  UserService service;
  UserLogger logger;
};
