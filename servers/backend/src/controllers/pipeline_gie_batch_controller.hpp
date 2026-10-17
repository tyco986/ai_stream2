#pragma once

#include "constants/pipeline_gie_const.hpp"
#include "log/pipeline_gie_logger.hpp"
#include "services/pipeline_gie_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>
#include <functional>

class PipelineGieBatchController : public drogon::HttpController<PipelineGieBatchController> {
 public:
  PipelineGieBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PipelineGieBatchController::batchRemove, pipeline_gie::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  PipelineGieService service;
  PipelineGieLogger logger;
};
