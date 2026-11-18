#include "controllers/media_controller.hpp"

#include "constants/ffmpeg_const.hpp"
#include "constants/media_const.hpp"
#include "constants/stream_const.hpp"
#include "db/database.hpp"
#include "ffmpeg/ffmpeg.hpp"
#include "mediamtx/mediamtx.hpp"
#include "response/response.hpp"

#include <exception>
#include <filesystem>

MediaController::MediaController() : service(Database::client()), streams(Database::client()) {}

std::string MediaController::publishName(const std::filesystem::path& file) {
  const std::string full = file.string();
  const auto index = full.rfind(media::PUBLISH_MARKER);
  const std::string name = index == std::string::npos
      ? full
      : full.substr(index + std::char_traits<char>::length(media::PUBLISH_MARKER));
  return name;
}

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
  logger.record(request, response, started, errorText);
  callback(response);
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
                                                       request->getParameter(media::QUERY_FOLDER)),
                                  media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.record(request, response, started, errorText);
  callback(response);
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
                                                        request->getParameter(media::QUERY_FOLDER),
                                                        request->getParameter(media::QUERY_DATE)),
                                  media::MSG_LIST_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, media::MSG_LIST_ERROR);
    errorText = error.what();
  }
  logger.record(request, response, started, errorText);
  callback(response);
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
  logger.record(request, response, started, errorText);
  callback(response);
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
  logger.record(request, response, started, errorText);
  callback(response);
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
  logger.record(request, response, started, errorText);
  callback(response);
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
  logger.record(request, response, started, errorText);
  callback(response);
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
  logger.record(request, response, started, errorText);
  callback(response);
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
  logger.record(request, response, started, errorText);
  callback(response);
}

drogon::Task<drogon::HttpResponsePtr> MediaController::listPublished(drogon::HttpRequestPtr request) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, media::MSG_PUBLISH_LIST_ERROR);
  const ProbeResult listed = co_await Ffmpeg::rtspPublished();
  response = listed.accepted
      ? http::Response::ok(listed.data, media::MSG_PUBLISH_LIST_OK)
      : response;
  logger.record(request, response, started, "");
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> MediaController::publish(drogon::HttpRequestPtr request,
                                                               std::string media_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, media::MSG_PUBLISH_ERROR);
  std::string errorText;
  bool ready = false;
  std::string name;
  std::string path;
  std::string streamName;
  try {
    const Json::Value row = service.get(media_id);
    const std::filesystem::path file = service.mediaId2Path(media_id);
    ready = row[media::KEY_TYPE].asString() == media::TYPE_VIDEO;
    name = publishName(file);
    path = file.string();
    streamName = row[media::KEY_NAME].asString();
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  if (ready) {
    const ProbeResult published = co_await Ffmpeg::rtspPublish(name, path);
    const Json::Value url = published.accepted ? published.data[ffmpeg_backend::KEY_URL_DOCKER] : Json::Value();
    const bool mounted = url.isString() &&
        co_await MediaMtx::replacePath(streamName, url.asString(), false) == PathReplace::mounted;
    if (mounted) {
      try {
        streams.create(streamName,
                       stream::ALL_GROUP_ID,
                       url,
                       true,
                       false,
                       published.data[ffmpeg_backend::KEY_STATUS],
                       published.data[stream::KEY_WIDTH],
                       published.data[stream::KEY_HEIGHT],
                       published.data[stream::KEY_FPS],
                       published.data[ffmpeg_backend::KEY_PROBED_AT]);
        response = http::Response::ok(published.data, media::MSG_PUBLISH_OK);
      } catch (const std::exception& error) {
        errorText = error.what();
      }
    }
  }
  logger.record(request, response, started, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> MediaController::unpublish(drogon::HttpRequestPtr request,
                                                                 std::string media_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, media::MSG_UNPUBLISH_ERROR);
  std::string errorText;
  bool ready = false;
  std::string name;
  try {
    const Json::Value row = service.get(media_id);
    const std::filesystem::path file = service.mediaId2Path(media_id);
    ready = row[media::KEY_TYPE].asString() == media::TYPE_VIDEO;
    name = publishName(file);
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  if (ready) {
    const ProbeResult removed = co_await Ffmpeg::rtspUnpublish(name);
    response = removed.accepted
        ? http::Response::ok(removed.data, media::MSG_UNPUBLISH_OK)
        : response;
  }
  logger.record(request, response, started, errorText);
  co_return response;
}
