#pragma once

#include "constants/pipeline_analyzer_const.hpp"
#include "log/pipeline_analyzer_logger.hpp"
#include "services/pipeline_analyzer_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>
#include <functional>

class PipelineAnalyzerBatchController : public drogon::HttpController<PipelineAnalyzerBatchController> {
 public:
  PipelineAnalyzerBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(
      PipelineAnalyzerBatchController::batchRemove, pipeline_analyzer::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  PipelineAnalyzerService service;
  PipelineAnalyzerLogger logger;
};
