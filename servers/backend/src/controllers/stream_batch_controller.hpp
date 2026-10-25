#pragma once

#include "constants/stream_const.hpp"
#include "logger/stream_logger.hpp"
#include "services/stream_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>


class StreamBatchController : public drogon::HttpController<StreamBatchController> {
 public:
  StreamBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamBatchController::batchCreate, stream::PATH_BATCH_CREATE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchRemove, stream::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchMount, stream::PATH_BATCH_MOUNT, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchUnmount, stream::PATH_BATCH_UNMOUNT, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchRecord, stream::PATH_BATCH_RECORD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchUnrecord, stream::PATH_BATCH_UNRECORD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamBatchController::batchProbe, stream::PATH_BATCH_PROBE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  drogon::Task<drogon::HttpResponsePtr> batchCreate(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchRemove(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchMount(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchUnmount(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchRecord(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchUnrecord(drogon::HttpRequestPtr request);
  drogon::Task<drogon::HttpResponsePtr> batchProbe(drogon::HttpRequestPtr request);

 private:
  StreamService service;
  StreamLogger logger;
};
