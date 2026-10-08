#pragma once

#include "constants/stream.hpp"
#include "log/stream_logger.hpp"

#include <drogon/HttpController.h>

#include <chrono>

class StreamBatchController : public drogon::HttpController<StreamBatchController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamBatchController::batchRemove, stream::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchEnable, stream::PATH_BATCH_ENABLE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchDisable, stream::PATH_BATCH_DISABLE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchRecord, stream::PATH_BATCH_RECORD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchUnrecord, stream::PATH_BATCH_UNRECORD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchProbe, stream::PATH_BATCH_PROBE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchEnable(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchDisable(const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchRecord(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchUnrecord(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchProbe(const drogon::HttpRequestPtr& request,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback);

  StreamLogger logger;
};
