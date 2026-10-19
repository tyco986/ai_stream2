#pragma once

#include "constants/user_const.hpp"
#include "log/user_group_logger.hpp"
#include "services/user_group_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>

class UserGroupController : public drogon::HttpController<UserGroupController> {
 public:
  UserGroupController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(UserGroupController::list, user::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::map, user::PATH_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::create, user::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::get, user::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::update, user::PATH_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::remove, user::PATH_ITEM, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::members, user::PATH_MEMBERS, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::candidates, user::PATH_CANDIDATES, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(UserGroupController::setMembers, user::PATH_MEMBERS, drogon::Put, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& group_id);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& group_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& group_id);
  void members(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& group_id);
  void candidates(const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& group_id);
  void setMembers(const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                  const std::string& group_id);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  UserGroupService service;
  UserGroupLogger logger;
};
