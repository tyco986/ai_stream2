#pragma once

#include "constants/pipeline_analyzer_const.hpp"
#include "log/pipeline_analyzer_logger.hpp"
#include "services/pipeline_analyzer_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>
#include <functional>
#include <string>

struct AnalyzerForm {
  Json::Value name;
  Json::Value annotations;
  std::string fileName;
  std::string fileContent;
};

class PipelineAnalyzerController : public drogon::HttpController<PipelineAnalyzerController> {
 public:
  PipelineAnalyzerController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PipelineAnalyzerController::list, pipeline_analyzer::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineAnalyzerController::get, pipeline_analyzer::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PipelineAnalyzerController::create, pipeline_analyzer::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineAnalyzerController::update, pipeline_analyzer::PATH_ITEM, drogon::Put, "AuthFilter");
  ADD_METHOD_TO(PipelineAnalyzerController::remove, pipeline_analyzer::PATH_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& analyzer_id);
  void create(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& analyzer_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& analyzer_id);

 private:
  AnalyzerForm multipart2Form(const drogon::HttpRequestPtr& request);
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  PipelineAnalyzerService service;
  PipelineAnalyzerLogger logger;
};
