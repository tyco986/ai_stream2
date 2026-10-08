#pragma once

#include "constants/stream_group.hpp"
#include "log/stream_group_logger.hpp"

#include <drogon/HttpController.h>

#include <chrono>

class StreamGroupController : public drogon::HttpController<StreamGroupController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamGroupController::tree, stream_group::PATH_TREE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::map, stream_group::PATH_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::create, stream_group::PATH_GROUP_CREATE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::rename, stream_group::PATH_GROUP, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::remove, stream_group::PATH_GROUP, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::members, stream_group::PATH_GROUP_MEMBERS, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::candidates,
                stream_group::PATH_GROUP_CANDIDATES,
                drogon::Get,
                "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::setMembers, stream_group::PATH_GROUP_MEMBERS, drogon::Put, "AuthFilter");
  METHOD_LIST_END

  void tree(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& parent_group_id);
  void rename(const drogon::HttpRequestPtr& request,
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
               const std::function<void(const drogon::HttpResponsePtr&)>& callback);

  StreamGroupLogger logger;
};
