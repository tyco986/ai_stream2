#pragma once

#include "constants/pipeline_analyzer_const.hpp"
#include "logger/pipeline_analyzer_logger.hpp"
#include "services/pipeline_analyzer_service.hpp"

#include <drogon/HttpController.h>

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
  PipelineAnalyzerService service;
  PipelineAnalyzerLogger logger;
};
