#pragma once

#include "constants/stream.hpp"
#include "log/stream_logger.hpp"

#include <drogon/HttpController.h>

#include <chrono>

class StreamController : public drogon::HttpController<StreamController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(StreamController::list, stream::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamController::create, stream::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(StreamController::get, stream::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(StreamController::update, stream::PATH_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(StreamController::remove, stream::PATH_ITEM, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(StreamController::probe, stream::PATH_PROBE, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& stream_id);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& stream_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& stream_id);
  void probe(const drogon::HttpRequestPtr& request,
             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
             const std::string& stream_id);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback);

  StreamLogger logger;
};
