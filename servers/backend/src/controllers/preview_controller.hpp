#pragma once

#include "constants/preview_const.hpp"
#include "logger/preview_logger.hpp"
#include "services/preview_service.hpp"

#include <drogon/HttpController.h>


class PreviewController : public drogon::HttpController<PreviewController> {
 public:
  PreviewController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(PreviewController::map, preview::PATH_MAP, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PreviewController::tree, preview::PATH_TREE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PreviewController::getActive, preview::PATH_ACTIVE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PreviewController::get, preview::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(PreviewController::create, preview::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(PreviewController::update, preview::PATH_ITEM, drogon::Patch, "AuthFilter");
  ADD_METHOD_TO(PreviewController::remove, preview::PATH_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void map(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void tree(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void getActive(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& preset_id);
  void create(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void update(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& preset_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& preset_id);

 private:
  PreviewService service;
  PreviewLogger logger;
};
