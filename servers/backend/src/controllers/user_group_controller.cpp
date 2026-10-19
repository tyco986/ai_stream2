#include "controllers/user_group_controller.hpp"

#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

UserGroupController::UserGroupController() : service(Database::client()) {}

void UserGroupController::respond(
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

void UserGroupController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(user::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(user::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? user::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? user::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    respond(request,
            http::Response::ok(service.list(request->getParameter(user::QUERY_SEARCH), page, pageSize),
                               user::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, user::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void UserGroupController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.map(), user::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, user::MSG_MAP_ERROR),
            started,
            callback,
            error.what());
  }
}

void UserGroupController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.get(group_id), user::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_NOT_FOUND;
    respond(request,
            http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_NOT_FOUND : user::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

void UserGroupController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    respond(request,
            http::Response::ok(service.create(json[user::KEY_NAME],
                                              json[user::KEY_STREAM_CRUD],
                                              json[user::KEY_MODEL_CRUD],
                                              json[user::KEY_PREVIEW_CRUD],
                                              json[user::KEY_PIPELINE_CRUD],
                                              json[user::KEY_RECORDING_CRUD],
                                              json[user::KEY_SERVER_CRUD],
                                              json[user::KEY_USER_CRUD],
                                              json[user::KEY_EVENT_CRUD]),
                               user::MSG_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, user::MSG_CREATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void UserGroupController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    respond(request,
            http::Response::ok(service.update(group_id,
                                              json[user::KEY_NAME],
                                              json[user::KEY_STREAM_CRUD],
                                              json[user::KEY_MODEL_CRUD],
                                              json[user::KEY_PREVIEW_CRUD],
                                              json[user::KEY_PIPELINE_CRUD],
                                              json[user::KEY_RECORDING_CRUD],
                                              json[user::KEY_SERVER_CRUD],
                                              json[user::KEY_USER_CRUD],
                                              json[user::KEY_EVENT_CRUD]),
                               user::MSG_UPDATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_GROUP_LOCKED;
    const bool missing = reason == user::ERR_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_GROUP_LOCKED : missing ? user::ERR_NOT_FOUND : user::MSG_UPDATE_ERROR;
    respond(request, http::Response::error(code, message), started, callback, reason);
  }
}

void UserGroupController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.remove(group_id), user::MSG_REMOVE_OK), started, callback);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_GROUP_LOCKED;
    const bool missing = reason == user::ERR_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_GROUP_LOCKED : missing ? user::ERR_NOT_FOUND : user::MSG_REMOVE_ERROR;
    respond(request, http::Response::error(code, message), started, callback, reason);
  }
}

void UserGroupController::members(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.members(group_id), user::MSG_MEMBERS_OK), started, callback);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_NOT_FOUND;
    respond(request,
            http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_NOT_FOUND : user::MSG_MEMBERS_ERROR),
            started,
            callback,
            error.what());
  }
}

void UserGroupController::candidates(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.candidates(group_id), user::MSG_CANDIDATES_OK), started, callback);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_NOT_FOUND;
    respond(request,
            http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_NOT_FOUND : user::MSG_CANDIDATES_ERROR),
            started,
            callback,
            error.what());
  }
}

void UserGroupController::setMembers(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value userIds = body ? (*body)[user::KEY_USER_IDS] : Json::Value(Json::arrayValue);
    respond(request,
            http::Response::ok(service.setMembers(group_id, userIds), user::MSG_SET_MEMBERS_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_USER_LOCKED || reason == user::ERR_GROUP_LOCKED;
    const bool missing = reason == user::ERR_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_USER_LOCKED : missing ? user::ERR_NOT_FOUND : user::MSG_SET_MEMBERS_ERROR;
    respond(request, http::Response::error(code, message), started, callback, reason);
  }
}
