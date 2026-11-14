#include "controllers/preview_controller.hpp"

#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <exception>

PreviewController::PreviewController() : service(Database::client()) {}

void PreviewController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.map(service.token2UserId(request->getHeader("authorization"))), preview::MSG_MAP_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_MAP_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PreviewController::tree(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.tree(), preview::MSG_TREE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_TREE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PreviewController::getActive(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.getActive(service.token2UserId(request->getHeader("authorization"))), preview::MSG_ACTIVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_ACTIVE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PreviewController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& preset_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.get(preset_id, service.token2UserId(request->getHeader("authorization"))), preview::MSG_GET_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_GET_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PreviewController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    const auto response = http::Response::ok(service.create(service.token2UserId(request->getHeader("authorization")),
                                              json[preview::KEY_NAME],
                                              json[preview::KEY_LAYOUT],
                                              json[preview::KEY_MODE],
                                              json[preview::KEY_SLOTS],
                                              json[preview::KEY_ACTIVE]),
                               preview::MSG_CREATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_CREATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PreviewController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& preset_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto body = request->getJsonObject();
    const Json::Value empty;
    const Json::Value& json = body ? *body : empty;
    const auto response = http::Response::ok(service.update(preset_id,
                                              service.token2UserId(request->getHeader("authorization")),
                                              json[preview::KEY_NAME],
                                              json[preview::KEY_LAYOUT],
                                              json[preview::KEY_MODE],
                                              json[preview::KEY_SLOTS],
                                              json[preview::KEY_ACTIVE]),
                               preview::MSG_UPDATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_UPDATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PreviewController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& preset_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.remove(preset_id, service.token2UserId(request->getHeader("authorization"))), preview::MSG_REMOVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, preview::MSG_REMOVE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}
