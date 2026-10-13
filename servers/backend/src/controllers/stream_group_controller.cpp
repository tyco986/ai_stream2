#include "controllers/stream_group_controller.hpp"

#include "constants/stream_group_const.hpp"
#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <drogon/MultiPart.h>

#include <chrono>
#include <exception>
#include <vector>

StreamGroupController::StreamGroupController() : service(Database::client()) {}

void StreamGroupController::respond(
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

void StreamGroupController::importCSV(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::MultiPartParser parser;
  const bool parsed = parser.parse(request) == 0;
  std::string fileContent;
  for (const auto& file : parser.getFiles()) {
    const bool matched = file.getItemName() == stream_group::FIELD_FILE;
    fileContent = matched ? std::string(file.fileData(), file.fileLength()) : fileContent;
  }
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k400BadRequest, stream_group::MSG_IMPORT_ERROR);
  std::string errorText;
  try {
    if (parsed && !fileContent.empty()) {
      response = http::Response::ok(service.importCSV(fileContent), stream_group::MSG_IMPORT_OK);
    }
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool rejected = reason == stream_group::ERR_CSV || reason == stream_group::ERR_PARENT_MISSING ||
        reason == stream_group::ERR_GROUP_EXISTS || reason == stream_group::ERR_ALL;
    response = http::Response::error(rejected ? drogon::k400BadRequest : drogon::k500InternalServerError,
                                     rejected ? reason.c_str() : stream_group::MSG_IMPORT_ERROR);
    errorText = reason;
  }
  respond(request, response, started, callback, errorText);
}

void StreamGroupController::exportCSV(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response =
      http::Response::error(drogon::k500InternalServerError, stream_group::MSG_EXPORT_ERROR);
  std::string errorText;
  try {
    const std::string csv = service.exportCSV();
    const std::string body = "--" + std::string(stream_group::MULTIPART_BOUNDARY) + "\r\n"
        "Content-Disposition: form-data; name=\"" + std::string(stream_group::FIELD_FILE) +
        "\"; filename=\"" + std::string(stream_group::EXPORT_FILENAME) + "\"\r\n"
        "Content-Type: text/csv\r\n\r\n" +
        csv + "\r\n--" + std::string(stream_group::MULTIPART_BOUNDARY) + "--\r\n";
    response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(drogon::k200OK);
    response->setContentTypeString(stream_group::MULTIPART_CONTENT_TYPE);
    response->setBody(body);
  } catch (const std::exception& error) {
    errorText = error.what();
  }
  respond(request, response, started, callback, errorText);
}

void StreamGroupController::tree(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.tree(), stream_group::MSG_TREE_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_TREE_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& parent_group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const std::string name = body ? (*body)[stream_group::KEY_NAME].asString() : "";
    respond(request,
            http::Response::ok(service.create(parent_group_id, name), stream_group::MSG_GROUP_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_CREATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::rename(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const std::string name = body ? (*body)[stream_group::KEY_NAME].asString() : "";
    respond(request,
            http::Response::ok(service.rename(group_id, name), stream_group::MSG_GROUP_RENAME_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_RENAME_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request,
            http::Response::ok(service.remove(group_id), stream_group::MSG_GROUP_DELETE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_DELETE_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::members(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request,
            http::Response::ok(service.members(group_id), stream_group::MSG_GROUP_MEMBERS_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_MEMBERS_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::candidates(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request,
            http::Response::ok(service.candidates(group_id), stream_group::MSG_GROUP_CANDIDATES_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_CANDIDATES_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::setMembers(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    std::vector<std::string> streamIds;
    if (body && (*body)[stream_group::KEY_STREAM_IDS].isArray()) {
      for (const auto& id : (*body)[stream_group::KEY_STREAM_IDS]) {
        streamIds.push_back(id.asString());
      }
    }
    respond(request,
            http::Response::ok(service.setMembers(group_id, streamIds), stream_group::MSG_GROUP_SET_MEMBERS_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_SET_MEMBERS_ERROR),
            started,
            callback,
            error.what());
  }
}

void StreamGroupController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.map(), stream_group::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_MAP_ERROR),
            started,
            callback,
            error.what());
  }
}
