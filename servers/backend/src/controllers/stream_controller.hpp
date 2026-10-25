#pragma once

#include "constants/stream_const.hpp"
#include "logger/stream_logger.hpp"
#include "services/stream_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>


class StreamController : public drogon::HttpController<StreamController> {
 public:
  StreamController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamController::list, stream::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamController::create, stream::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamController::importCSV, stream::PATH_IMPORT, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamController::exportCSV, stream::PATH_EXPORT, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamController::get, stream::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamController::update, stream::PATH_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(StreamController::remove, stream::PATH_ITEM, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(StreamController::probe, stream::PATH_PROBE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr request);
  void importCSV(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void exportCSV(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& stream_id);
  drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr request,
                                               std::string stream_id);
  drogon::Task<drogon::HttpResponsePtr> remove(drogon::HttpRequestPtr request,
                                               std::string stream_id);
  drogon::Task<drogon::HttpResponsePtr> probe(drogon::HttpRequestPtr request,
                                              std::string stream_id);

 private:
  StreamService service;
  StreamLogger logger;
};
