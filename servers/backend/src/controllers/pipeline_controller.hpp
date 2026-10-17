#pragma once

#include "constants/pipeline_const.hpp"
#include "log/pipeline_logger.hpp"
#include "services/pipeline_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <functional>
#include <string>

class PipelineController : public drogon::HttpController<PipelineController> {
 public:
  PipelineController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PipelineController::list, pipeline::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineController::map, pipeline::PATH_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineController::get, pipeline::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineController::create, pipeline::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineController::update, pipeline::PATH_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(PipelineController::remove, pipeline::PATH_ITEM, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(PipelineController::start, pipeline::PATH_START, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineController::probe, pipeline::PATH_PROBE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineController::stop, pipeline::PATH_STOP, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> create(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> update(const drogon::HttpRequestPtr& request, const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> remove(const drogon::HttpRequestPtr& request, const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> start(const drogon::HttpRequestPtr& request, const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> probe(const drogon::HttpRequestPtr& request, const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> stop(const drogon::HttpRequestPtr& request, const std::string& pipeline_id);

 private:
  std::string yaml2Multipart(const std::string& yaml);
  std::string zip2Multipart(const std::string& zip);
  drogon::Task<Json::Value> postGenerate(const std::string& yaml);
  drogon::Task<bool> runDeepstreamContainer(const std::string& containerName);
  drogon::Task<Json::Value> postDeepstreamStart(const std::string& containerName, const std::string& configPath);
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  PipelineService service;
  PipelineLogger logger;
};
