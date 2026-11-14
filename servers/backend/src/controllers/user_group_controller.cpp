#include "controllers/user_group_controller.hpp"

#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

UserGroupController::UserGroupController() : service(Database::client()) {}

void UserGroupController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(user::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(user::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? user::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? user::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    const auto response = http::Response::ok(service.list(request->getParameter(user::QUERY_SEARCH), page, pageSize),
                               user::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, user::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserGroupController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.map(), user::MSG_MAP_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, user::MSG_MAP_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserGroupController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.get(group_id), user::MSG_GET_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_NOT_FOUND;
    const auto response = http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_NOT_FOUND : user::MSG_GET_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
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
    const auto response = http::Response::ok(service.create(json[user::KEY_NAME],
                                              json[user::KEY_STREAM_CRUD],
                                              json[user::KEY_MODEL_CRUD],
                                              json[user::KEY_PREVIEW_CRUD],
                                              json[user::KEY_PIPELINE_CRUD],
                                              json[user::KEY_RECORDING_CRUD],
                                              json[user::KEY_SERVER_CRUD],
                                              json[user::KEY_USER_CRUD],
                                              json[user::KEY_EVENT_CRUD]),
                               user::MSG_CREATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, user::MSG_CREATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
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
    const auto response = http::Response::ok(service.update(group_id,
                                              json[user::KEY_NAME],
                                              json[user::KEY_STREAM_CRUD],
                                              json[user::KEY_MODEL_CRUD],
                                              json[user::KEY_PREVIEW_CRUD],
                                              json[user::KEY_PIPELINE_CRUD],
                                              json[user::KEY_RECORDING_CRUD],
                                              json[user::KEY_SERVER_CRUD],
                                              json[user::KEY_USER_CRUD],
                                              json[user::KEY_EVENT_CRUD]),
                               user::MSG_UPDATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_GROUP_LOCKED;
    const bool missing = reason == user::ERR_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_GROUP_LOCKED : missing ? user::ERR_NOT_FOUND : user::MSG_UPDATE_ERROR;
    const auto response = http::Response::error(code, message);
    logger.record(request, response, started, reason);
    callback(response);
  }
}

void UserGroupController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.remove(group_id), user::MSG_REMOVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_GROUP_LOCKED;
    const bool missing = reason == user::ERR_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_GROUP_LOCKED : missing ? user::ERR_NOT_FOUND : user::MSG_REMOVE_ERROR;
    const auto response = http::Response::error(code, message);
    logger.record(request, response, started, reason);
    callback(response);
  }
}

void UserGroupController::members(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.members(group_id), user::MSG_MEMBERS_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_NOT_FOUND;
    const auto response = http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_NOT_FOUND : user::MSG_MEMBERS_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserGroupController::candidates(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& group_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.candidates(group_id), user::MSG_CANDIDATES_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_NOT_FOUND;
    const auto response = http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_NOT_FOUND : user::MSG_CANDIDATES_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
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
    const auto response = http::Response::ok(service.setMembers(group_id, userIds), user::MSG_SET_MEMBERS_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_USER_LOCKED || reason == user::ERR_GROUP_LOCKED;
    const bool missing = reason == user::ERR_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_USER_LOCKED : missing ? user::ERR_NOT_FOUND : user::MSG_SET_MEMBERS_ERROR;
    const auto response = http::Response::error(code, message);
    logger.record(request, response, started, reason);
    callback(response);
  }
}
