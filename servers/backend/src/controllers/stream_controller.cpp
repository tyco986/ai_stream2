#include "controllers/stream_controller.hpp"

#include "constants/stream_const.hpp"
#include "db/database.hpp"
#include "ffmpeg/ffmpeg.hpp"
#include "mediamtx/mediamtx.hpp"
#include "response/response.hpp"
#include "log/base_logger.hpp"

#include <drogon/MultiPart.h>

#include <chrono>
#include <exception>

StreamController::StreamController() : service(Database::client()) {}

void StreamController::respond(
    const drogon::HttpRequestPtr& request,
    const drogon::HttpResponsePtr& response,
    std::chrono::steady_clock::time_point started,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback,
    const std::string& error) {
  const auto elapsed = std::chrono::steady_clock::now() - started;
  const std::string query = request->query();
  LogEntry entry;
  entry.method = request->methodString();
  entry.url = request->path();
  entry.url += query.empty() ? "" : "?" + query;
  entry.code = static_cast<int>(response->statusCode());
  entry.input = std::string(request->getBody());
  entry.output = std::string(response->getBody());
  entry.latency = std::chrono::duration<double, std::milli>(elapsed).count();
  entry.ip = request->peerAddr().toIp();
  entry.inBytes = request->bodyLength();
  entry.outBytes = response->getBodyLength();
  entry.error = error;
  logger.write(entry);
  callback(response);
}

void StreamController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(stream::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(stream::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? stream::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? stream::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    respond(request,
            http::Response::ok(
                service.list(request->getParameter(stream::QUERY_GROUP_ID),
                             request->getParameter(stream::QUERY_STREAM_ID),
                             request->getParameter(stream::QUERY_SEARCH),
                             page,
                             pageSize),
                stream::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

drogon::Task<drogon::HttpResponsePtr> StreamController::create(const drogon::HttpRequestPtr& request) {
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  const Json::Value name = payload.get(stream::KEY_NAME, Json::Value());
  const Json::Value groupId = payload.get(stream::KEY_GROUP_ID, Json::Value());
  const Json::Value url = payload.get(stream::KEY_URL, Json::Value());
  const Json::Value enabled = payload.get(stream::KEY_ENABLED, Json::Value());
  const Json::Value recording = payload.get(stream::KEY_RECORD, Json::Value());
  const Json::Value status = payload.get(stream::KEY_STATUS, Json::Value());
  const Json::Value width = payload.get(stream::KEY_WIDTH, Json::Value());
  const Json::Value height = payload.get(stream::KEY_HEIGHT, Json::Value());
  const Json::Value fps = payload.get(stream::KEY_FPS, Json::Value());
  const Json::Value probedAt = payload.get(stream::KEY_PROBED_AT, Json::Value());
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_CREATE_ERROR);
  std::string errorText;
  const bool mount = enabled.isNull() || enabled.asBool();
  bool mounted = true;
  if (mount) {
    mounted = co_await MediaMtx::replacePath(name.isString() ? name.asString() : "",
                                              url.isString() ? url.asString() : "",
                                              recording.isBool() && recording.asBool());
  }
  if (mounted) {
    try {
      response = http::Response::ok(
          service.create(name, groupId, url, enabled, recording, status, width, height, fps, probedAt),
          stream::MSG_CREATE_OK);
    } catch (const std::exception& error) {
      errorText = error.what();
    }
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamController::update(const drogon::HttpRequestPtr& request,
                                                                      const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  const Json::Value name = payload.get(stream::KEY_NAME, Json::Value());
  const Json::Value groupId = payload.get(stream::KEY_GROUP_ID, Json::Value());
  const Json::Value url = payload.get(stream::KEY_URL, Json::Value());
  const Json::Value enabled = payload.get(stream::KEY_ENABLED, Json::Value());
  const Json::Value recording = payload.get(stream::KEY_RECORD, Json::Value());
  const Json::Value status = payload.get(stream::KEY_STATUS, Json::Value());
  const Json::Value width = payload.get(stream::KEY_WIDTH, Json::Value());
  const Json::Value height = payload.get(stream::KEY_HEIGHT, Json::Value());
  const Json::Value fps = payload.get(stream::KEY_FPS, Json::Value());
  const Json::Value probedAt = payload.get(stream::KEY_PROBED_AT, Json::Value());
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_UPDATE_ERROR);
  std::string errorText;
  bool ready = false;
  std::string oldName;
  std::string nameText;
  std::string urlText;
  bool recordingFlag = false;
  bool enabledFlag = false;
  bool rename = false;
  bool pathChanged = false;
  try {
    const Json::Value current = service.get(stream_id);
    oldName = current[stream::KEY_NAME].asString();
    nameText = name.isNull() ? oldName : name.asString();
    urlText = url.isNull() ? current[stream::KEY_URL].asString() : url.asString();
    enabledFlag = enabled.isNull() ? current[stream::KEY_ENABLED].asBool() : enabled.asBool();
    recordingFlag = recording.isNull() ? current[stream::KEY_RECORD].asBool() : recording.asBool();
    rename = nameText != oldName;
    pathChanged = rename || urlText != current[stream::KEY_URL].asString() ||
        recordingFlag != current[stream::KEY_RECORD].asBool();
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  const bool mountedNow = ready && co_await MediaMtx::getPath(oldName);
  const bool unmount = mountedNow && (!enabledFlag || rename);
  const bool mount = enabledFlag && (!mountedNow || pathChanged);
  bool removed = ready;
  if (unmount) {
    removed = co_await MediaMtx::deletePath(oldName);
  }
  bool mounted = removed;
  if (removed && mount) {
    mounted = co_await MediaMtx::replacePath(nameText, urlText, recordingFlag);
  }
  if (mounted) {
    try {
      response = http::Response::ok(service.update(stream_id,
                                                   name,
                                                   groupId,
                                                   url,
                                                   enabled,
                                                   recording,
                                                   status,
                                                   width,
                                                   height,
                                                   fps,
                                                   probedAt),
                                    stream::MSG_UPDATE_OK);
    } catch (const std::exception& error) {
      errorText = error.what();
    }
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamController::remove(const drogon::HttpRequestPtr& request,
                                                                      const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_DELETE_ERROR);
  std::string errorText;
  bool ready = false;
  std::string name;
  try {
    name = service.get(stream_id)[stream::KEY_NAME].asString();
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  bool removed = ready;
  if (ready && co_await MediaMtx::getPath(name)) {
    removed = co_await MediaMtx::deletePath(name);
  }
  if (removed) {
    try {
      service.remove(stream_id);
      response = http::Response::ok(Json::Value(Json::nullValue), stream::MSG_DELETE_OK);
    } catch (const std::exception& error) {
      errorText = error.what();
    }
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

drogon::Task<drogon::HttpResponsePtr> StreamController::probe(const drogon::HttpRequestPtr& request,
                                                                     const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_PROBE_ERROR);
  std::string errorText;
  bool ready = false;
  std::string url;
  try {
    url = service.get(stream_id)[stream::KEY_URL].asString();
    ready = true;
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  if (ready) {
    const ProbeResult probed = co_await Ffmpeg::probe(url);
    if (probed.accepted) {
      try {
        response = http::Response::ok(service.probe(stream_id,
                                                    probed.data[stream::KEY_STATUS],
                                                    probed.data[stream::KEY_WIDTH],
                                                    probed.data[stream::KEY_HEIGHT],
                                                    probed.data[stream::KEY_FPS],
                                                    probed.data[stream::KEY_PROBED_AT]),
                                      stream::MSG_PROBE_OK);
      } catch (const std::exception& error) {
        errorText = error.what();
      }
    }
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}

void StreamController::importCSV(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  std::string fileContent;
  for (const auto& file : parser.getFiles()) {
    const bool matched = file.getItemName() == stream::FIELD_FILE;
    fileContent = matched ? std::string(file.fileData(), file.fileLength()) : fileContent;
  }
  drogon::HttpResponsePtr response = http::Response::error(drogon::k400BadRequest, stream::MSG_IMPORT_ERROR);
  std::string errorText;
  try {
    if (parsed && !fileContent.empty()) {
      response = http::Response::ok(service.importCSV(fileContent), stream::MSG_IMPORT_OK);
    }
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool rejected = reason == stream::ERR_CSV || reason == stream::ERR_GROUP_MISSING ||
        reason == stream::ERR_NAME_EXISTS || reason == stream::ERR_URL_EXISTS;
    response = http::Response::error(rejected ? drogon::k400BadRequest : drogon::k500InternalServerError,
                                     rejected ? reason.c_str() : stream::MSG_IMPORT_ERROR);
    errorText = reason;
  }
  respond(request, response, started, callback, errorText);
}

void StreamController::exportCSV(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream::MSG_EXPORT_ERROR);
  std::string errorText;
  try {
    const std::string csv = service.exportCSV();
    const std::string body = "--" + std::string(stream::MULTIPART_BOUNDARY) + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(stream::FIELD_FILE) +
        "\"; filename=\"" + std::string(stream::EXPORT_FILENAME) + "\"\r\n"
        "Content-Type: text/csv\r\n\r\n" +
        csv + "\r\n--" + std::string(stream::MULTIPART_BOUNDARY) + "--\r\n";
    response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k200OK);
    response->setContentTypeString(stream::MULTIPART_CONTENT_TYPE);
    response->setBody(body);
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  respond(request, response, started, callback, errorText);
}

void StreamController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& stream_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.get(stream_id), stream::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}
