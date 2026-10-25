#pragma once

#include "constants/media_const.hpp"
#include "logger/media_logger.hpp"
#include "services/media_service.hpp"

#include <drogon/HttpController.h>

#include <filesystem>
#include <functional>
#include <utility>
#include <vector>

class MediaBatchController : public drogon::HttpController<MediaBatchController> {
 public:
  MediaBatchController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(MediaBatchController::batchCreate, media::PATH_BATCH_CREATE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(MediaBatchController::batchUpload, media::PATH_BATCH_UPLOAD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(MediaBatchController::batchRemove, media::PATH_BATCH_REMOVE, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(MediaBatchController::batchDownload, media::PATH_BATCH_DOWNLOAD, drogon::Post, "AuthFilter");
  METHOD_LIST_END

  void batchCreate(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchUpload(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchRemove(const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void batchDownload(const drogon::HttpRequestPtr& request,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);

 private:
  std::filesystem::path zipPaths(const std::vector<std::pair<std::string, std::filesystem::path>>& files);
  MediaService service;
  MediaLogger logger;
};
