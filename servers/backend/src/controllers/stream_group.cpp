#include "controllers/stream_group.hpp"

#include "constants/stream_group.hpp"
#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"
#include "services/stream_group.hpp"

#include <chrono>
#include <exception>
#include <vector>

void StreamGroupController::respond(
    const drogon::HttpRequestPtr& request,
    const drogon::HttpResponsePtr& response,
    std::chrono::steady_clock::time_point started,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
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
  logger.write(entry);
  callback(response);
}

void StreamGroupController::tree(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    StreamGroupService service(Database::client());
    respond(request, http::Response::ok(service.tree(), stream_group::MSG_TREE_OK), started, callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_TREE_ERROR),
            started,
            callback);
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
    StreamGroupService service(Database::client());
    respond(request,
            http::Response::ok(service.create(parent_group_id, name), stream_group::MSG_GROUP_CREATE_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_CREATE_ERROR),
            started,
            callback);
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
    StreamGroupService service(Database::client());
    respond(request,
            http::Response::ok(service.rename(group_id, name), stream_group::MSG_GROUP_RENAME_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_RENAME_ERROR),
            started,
            callback);
  }
}

void StreamGroupController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    StreamGroupService service(Database::client());
    respond(request,
            http::Response::ok(service.remove(group_id), stream_group::MSG_GROUP_DELETE_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_DELETE_ERROR),
            started,
            callback);
  }
}

void StreamGroupController::members(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    StreamGroupService service(Database::client());
    respond(request,
            http::Response::ok(service.members(group_id), stream_group::MSG_GROUP_MEMBERS_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_MEMBERS_ERROR),
            started,
            callback);
  }
}

void StreamGroupController::candidates(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    StreamGroupService service(Database::client());
    respond(request,
            http::Response::ok(service.candidates(group_id), stream_group::MSG_GROUP_CANDIDATES_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_CANDIDATES_ERROR),
            started,
            callback);
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
    StreamGroupService service(Database::client());
    respond(request,
            http::Response::ok(service.setMembers(group_id, streamIds), stream_group::MSG_GROUP_SET_MEMBERS_OK),
            started,
            callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_GROUP_SET_MEMBERS_ERROR),
            started,
            callback);
  }
}

void StreamGroupController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    StreamGroupService service(Database::client());
    respond(request, http::Response::ok(service.map(), stream_group::MSG_MAP_OK), started, callback);
  } catch (const std::exception&) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, stream_group::MSG_MAP_ERROR),
            started,
            callback);
  }
}
