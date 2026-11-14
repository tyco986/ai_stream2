#include "controllers/user_controller.hpp"

#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

UserController::UserController() : service(Database::client()) {}

void UserController::login(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    const auto response = http::Response::ok(service.login(json[user::KEY_USERNAME], json[user::KEY_PASSWD]), user::MSG_LOGIN_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool rejected = std::string(error.what()) == user::ERR_LOGIN;
    const auto response = http::Response::error(rejected ? drogon::k401Unauthorized : drogon::k500InternalServerError,
                                  rejected ? user::ERR_LOGIN : user::MSG_LOGIN_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(user::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(user::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? user::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? user::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    const auto response = http::Response::ok(service.list(request->getParameter(user::QUERY_SEARCH), page, pageSize),
                               user::MSG_USER_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, user::MSG_USER_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.map(), user::MSG_USER_MAP_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, user::MSG_USER_MAP_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserController::me(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.me(request->getHeader("authorization")), user::MSG_USER_GET_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool rejected = std::string(error.what()) == user::ERR_UNAUTHORIZED;
    const auto response = http::Response::error(rejected ? drogon::k401Unauthorized : drogon::k500InternalServerError,
                                  rejected ? user::ERR_UNAUTHORIZED : user::MSG_USER_GET_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& user_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.get(user_id), user::MSG_USER_GET_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool missing = std::string(error.what()) == user::ERR_USER_NOT_FOUND;
    const auto response = http::Response::error(missing ? drogon::k404NotFound : drogon::k500InternalServerError,
                                  missing ? user::ERR_USER_NOT_FOUND : user::MSG_USER_GET_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    const auto response = http::Response::ok(service.create(json[user::KEY_USERNAME],
                                              json[user::KEY_PASSWD],
                                              json[user::KEY_ACTIVE],
                                              json[user::KEY_GROUP_ID]),
                               user::MSG_USER_CREATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, user::MSG_USER_CREATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void UserController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& user_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    const auto response = http::Response::ok(service.update(user_id,
                                              json[user::KEY_USERNAME],
                                              json[user::KEY_PASSWD],
                                              json[user::KEY_ACTIVE],
                                              json[user::KEY_GROUP_ID]),
                               user::MSG_USER_UPDATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_USER_LOCKED;
    const bool missing = reason == user::ERR_USER_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_USER_LOCKED : missing ? user::ERR_USER_NOT_FOUND : user::MSG_USER_UPDATE_ERROR;
    const auto response = http::Response::error(code, message);
    logger.record(request, response, started, reason);
    callback(response);
  }
}

void UserController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& user_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.remove(user_id), user::MSG_USER_REMOVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const std::string reason = error.what();
    const bool locked = reason == user::ERR_USER_LOCKED;
    const bool missing = reason == user::ERR_USER_NOT_FOUND;
    const auto code = locked ? drogon::k409Conflict : missing ? drogon::k404NotFound : drogon::k500InternalServerError;
    const char* message = locked ? user::ERR_USER_LOCKED : missing ? user::ERR_USER_NOT_FOUND : user::MSG_USER_REMOVE_ERROR;
    const auto response = http::Response::error(code, message);
    logger.record(request, response, started, reason);
    callback(response);
  }
}
