#pragma once

#include "constants/media_const.hpp"
#include "logger/media_logger.hpp"
#include "services/media_service.hpp"

#include <drogon/HttpController.h>

#include <functional>

class MediaController : public drogon::HttpController<MediaController> {
 public:
  MediaController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(MediaController::listUploadFile, media::PATH_UPLOAD_FILE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::listPipelineFile, media::PATH_PIPELINE_FILE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::listRecordingFile, media::PATH_RECORDING_FILE, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::listUploadFolder, media::PATH_UPLOAD_FOLDER, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::listPipelineFolder, media::PATH_PIPELINE_FOLDER, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::listRecordingFolder, media::PATH_RECORDING_FOLDER, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::get, media::PATH_ITEM, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::content, media::PATH_CONTENT, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(MediaController::download, media::PATH_DOWNLOAD, drogon::Get, "AuthFilter");
  METHOD_LIST_END

  void listUploadFile(const drogon::HttpRequestPtr& request,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void listPipelineFile(const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void listRecordingFile(const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void listUploadFolder(const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void listPipelineFolder(const drogon::HttpRequestPtr& request,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void listRecordingFolder(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void get(const drogon::HttpRequestPtr& request,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
           const std::string& media_id);
  void content(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& media_id);
  void download(const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& media_id);

 private:
  MediaService service;
  MediaLogger logger;
};
