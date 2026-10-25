#pragma once

#include "constants/pipeline_const.hpp"
#include "logger/pipeline_logger.hpp"
#include "services/pipeline_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <functional>

class PipelineBatchController : public drogon::HttpController<PipelineBatchController> {
 public:
  PipelineBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PipelineBatchController::batchStop, pipeline::PATH_BATCH_STOP, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PipelineBatchController::batchRemove, pipeline::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  drogon::Task<drogon::HttpResponsePtr> batchStop(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchRemove(drogon::HttpRequestPtr request);

 private:
  PipelineService service;
  PipelineLogger logger;
};
