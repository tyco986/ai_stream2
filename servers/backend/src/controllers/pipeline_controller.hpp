#pragma once

#include "constants/pipeline_const.hpp"
#include "logger/pipeline_logger.hpp"
#include "services/pipeline_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <functional>
#include <string>
#include <vector>

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
  ADD_METHOD_TO(PipelineController::logsApi, pipeline::PATH_LOGS_API, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineController::logsPad, pipeline::PATH_LOGS_PAD, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineController::logsGstreamer, pipeline::PATH_LOGS_GSTREAMER, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineController::removeLogs, pipeline::PATH_LOGS, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void map(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr request, std::string pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> remove(drogon::HttpRequestPtr request, std::string pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> start(drogon::HttpRequestPtr request, std::string pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> probe(drogon::HttpRequestPtr request, std::string pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> stop(drogon::HttpRequestPtr request, std::string pipeline_id);
  void logsApi(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& pipeline_id);
  void logsPad(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& pipeline_id);
  void logsGstreamer(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& pipeline_id);
  drogon::Task<drogon::HttpResponsePtr> removeLogs(drogon::HttpRequestPtr request, std::string pipeline_id);

 private:
  struct ZipPart {
    std::string field;
    std::string filename;
    std::string content;
  };

  ZipPart filePart(const std::string& body);
  std::string yaml2Multipart(const std::string& name, const std::string& yaml);
  std::string zips2Multipart(const std::vector<ZipPart>& files);
  drogon::Task<Json::Value> postGenerate(const std::string& name, const std::string& yaml);
  std::string hostBind(const std::string& dir);
  drogon::Task<bool> runDeepstreamContainer(const std::string& containerName,
                                           const std::string& pipelineName,
                                           Json::Value& steps);
  drogon::Task<Json::Value> postDeepstreamStart(const std::string& containerName,
                                                const std::string& configPath,
                                                Json::Value& steps);
  PipelineService service;
  PipelineLogger logger;
};
