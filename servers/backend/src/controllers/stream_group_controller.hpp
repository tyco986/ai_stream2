#pragma once

#include "constants/stream_group_const.hpp"
#include "logger/stream_group_logger.hpp"
#include "services/stream_group_service.hpp"

#include <drogon/HttpController.h>


class StreamGroupController : public drogon::HttpController<StreamGroupController> {
 public:
  StreamGroupController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamGroupController::importCSV, stream_group::PATH_IMPORT, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::exportCSV, stream_group::PATH_EXPORT, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::tree, stream_group::PATH_TREE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::map, stream_group::PATH_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::create, stream_group::PATH_GROUP_CREATE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::update, stream_group::PATH_GROUP, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::remove, stream_group::PATH_GROUP, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::members, stream_group::PATH_GROUP_MEMBERS, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::candidates,
                stream_group::PATH_GROUP_CANDIDATES,
                drogon::Get,
                "AuthFilter");
  ADD_METHOD_TO(StreamGroupController::setMembers, stream_group::PATH_GROUP_MEMBERS, drogon::Put, "AuthFilter");
  METHOD_LIST_END

  void importCSV(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void exportCSV(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void tree(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& parent_group_id);
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
  StreamGroupService service;
  StreamGroupLogger logger;
};
