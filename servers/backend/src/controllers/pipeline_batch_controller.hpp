#pragma once

#include "constants/pipeline_const.hpp"
#include "log/pipeline_logger.hpp"
#include "services/pipeline_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <functional>

class PipelineBatchController : public drogon::HttpController<PipelineBatchController> {
 public:
  PipelineBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PipelineBatchController::batchStop, pipeline::PATH_BATCH_STOP, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineBatchController::batchRemove, pipeline::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  drogon::Task<drogon::HttpResponsePtr> batchStop(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> batchRemove(const drogon::HttpRequestPtr& request);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  PipelineService service;
  PipelineLogger logger;
};
