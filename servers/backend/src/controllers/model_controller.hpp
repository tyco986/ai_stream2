#pragma once

#include "constants/model_const.hpp"
#include "logger/model_logger.hpp"
#include "services/model_service.hpp"

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include <functional>

class ModelController : public drogon::HttpController<ModelController> {
 public:
  ModelController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ModelController::list, model::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(ModelController::create, model::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(ModelController::importModels, model::PATH_IMPORT, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(ModelController::get, model::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(ModelController::update, model::PATH_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(ModelController::remove, model::PATH_ITEM, drogon::Delete, "AuthFilter");
  ADD_METHOD_TO(ModelController::upload, model::PATH_UPLOAD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(ModelController::build, model::PATH_BUILD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(ModelController::status, model::PATH_STATUS, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(ModelController::map, model::PATH_MAP, drogon::Get, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void importModels(const drogon::HttpRequestPtr& request,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& model_id);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& model_id);
  void upload(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& model_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& model_id);
  drogon::Task<drogon::HttpResponsePtr> build(drogon::HttpRequestPtr request,
                                              std::string model_id);
  void status(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& model_id);
  void map(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  std::string zip2Bytes(const std::string& path);
  std::string zip2Multipart(const std::string& filename, const std::string& bytes);
  std::string yaml2Multipart(const std::string& yaml);
  ModelService service;
  ModelLogger logger;
};
