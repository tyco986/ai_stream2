#pragma once

#include "constants/stream_const.hpp"
#include "log/stream_logger.hpp"
#include "services/stream_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <chrono>

class StreamBatchController : public drogon::HttpController<StreamBatchController> {
 public:
  StreamBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamBatchController::batchRemove, stream::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchEnable, stream::PATH_BATCH_ENABLE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchDisable, stream::PATH_BATCH_DISABLE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchRecord, stream::PATH_BATCH_RECORD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchUnrecord, stream::PATH_BATCH_UNRECORD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchProbe, stream::PATH_BATCH_PROBE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  drogon::Task<drogon::HttpResponsePtr> batchRemove(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> batchEnable(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> batchDisable(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> batchRecord(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> batchUnrecord(const drogon::HttpRequestPtr& request);
  drogon::Task<drogon::HttpResponsePtr> batchProbe(const drogon::HttpRequestPtr& request);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  StreamService service;
  StreamLogger logger;
};
