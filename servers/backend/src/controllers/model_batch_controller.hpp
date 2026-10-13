#pragma once

#include "constants/model_const.hpp"
#include "log/model_logger.hpp"
#include "services/model_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <functional>

class ModelBatchController : public drogon::HttpController<ModelBatchController> {
 public:
  ModelBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ModelBatchController::batchBuild, model::PATH_BATCH_BUILD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(ModelBatchController::batchStatus, model::PATH_BATCH_STATUS, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(ModelBatchController::batchRemove, model::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  drogon::Task<drogon::HttpResponsePtr> batchBuild(const drogon::HttpRequestPtr& request);
  void batchStatus(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  std::string zip2Bytes(const std::string& path);
  std::string zip2Multipart(const std::string& filename, const std::string& bytes);
  std::string yaml2Multipart(const std::string& yaml);
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  ModelService service;
  ModelLogger logger;
};
