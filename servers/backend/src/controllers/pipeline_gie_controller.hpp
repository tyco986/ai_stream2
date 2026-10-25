#pragma once

#include "constants/pipeline_gie_const.hpp"
#include "logger/pipeline_gie_logger.hpp"
#include "services/pipeline_gie_service.hpp"

#include <drogon/HttpController.h>

#include <functional>
#include <string>

class PipelineGieController : public drogon::HttpController<PipelineGieController> {
 public:
  PipelineGieController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PipelineGieController::list, pipeline_gie::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineGieController::getModel, pipeline_gie::PATH_MODELS, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineGieController::map, pipeline_gie::PATH_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineGieController::get, pipeline_gie::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineGieController::create, pipeline_gie::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineGieController::update, pipeline_gie::PATH_ITEM, drogon::Put, "AuthFilter");
  ADD_METHOD_TO(PipelineGieController::remove, pipeline_gie::PATH_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void getModel(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& gie_id);
  void create(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& gie_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& gie_id);

 private:
  PipelineGieService service;
  PipelineGieLogger logger;
};
