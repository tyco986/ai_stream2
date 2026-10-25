#include "controllers/media_controller.hpp"

#include "constants/media_const.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>
#include <filesystem>

MediaController::MediaController() : service(Database::client()) {}

void MediaController::listUploadFile(const drogon::HttpRequestPtr& request,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const std::string pageText = request->getParameter(media::QUERY_PAGE);
  const std::string pageSizeText = request->getParameter(media::QUERY_PAGE_SIZE);
  const int page = pageText.empty() ? media::DEFAULT_PAGE : std::stoi(pageText);
  const int pageSize = pageSizeText.empty() ? media::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.listUploadFile(page,
                                               pageSize,
                                               request->getParameter(media::QUERY_USER_ID),
                                               request->getParameter(media::QUERY_TYPE),
                                               request->getParameter(media::QUERY_SEARCH)),
                                  media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::listPipelineFile(const drogon::HttpRequestPtr& request,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const std::string pageText = request->getParameter(media::QUERY_PAGE);
  const std::string pageSizeText = request->getParameter(media::QUERY_PAGE_SIZE);
  const int page = pageText.empty() ? media::DEFAULT_PAGE : std::stoi(pageText);
  const int pageSize = pageSizeText.empty() ? media::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.listPipelineFile(page,
                                                       pageSize,
                                                       request->getParameter(media::QUERY_TYPE),
                                                       request->getParameter(media::QUERY_SEARCH),
                                                       request->getParameter(media::QUERY_PIPELINE_ID)),
                                  media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::listRecordingFile(const drogon::HttpRequestPtr& request,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const std::string pageText = request->getParameter(media::QUERY_PAGE);
  const std::string pageSizeText = request->getParameter(media::QUERY_PAGE_SIZE);
  const int page = pageText.empty() ? media::DEFAULT_PAGE : std::stoi(pageText);
  const int pageSize = pageSizeText.empty() ? media::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.listRecordingFile(page,
                                                        pageSize,
                                                        request->getParameter(media::QUERY_TYPE),
                                                        request->getParameter(media::QUERY_SEARCH),
                                                        request->getParameter(media::QUERY_STREAM_ID)),
                                  media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::listUploadFolder(const drogon::HttpRequestPtr& request,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.listUploadFolder(), media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::listPipelineFolder(const drogon::HttpRequestPtr& request,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.listPipelineFolder(), media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::listRecordingFolder(const drogon::HttpRequestPtr& request,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.listRecordingFolder(), media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::get(const drogon::HttpRequestPtr& request,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const std::string& media_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.get(media_id), media::MSG_GET_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_GET_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::content(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& media_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = drogon::HttpResponse::newFileResponse(service.mediaId2Path(media_id).string());
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_GET_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}

void MediaController::download(const drogon::HttpRequestPtr& request,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& media_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    const std::filesystem::path file = service.mediaId2Path(media_id);
    response = drogon::HttpResponse::newFileResponse(file.string(), file.filename().string());
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_DOWNLOAD_ERROR);
    errorText = error.what();
  }
  logger.respond(request, response, started, callback, errorText);
}
