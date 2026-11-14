#include "controllers/log_controller.hpp"

#include "response/response.hpp"

#include <chrono>

void LogController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const std::string date = request->getParameter(service_log::QUERY_DATE);
  const std::string page = request->getParameter(service_log::QUERY_PAGE);
  const Json::Value data = BaseLogger::get(page, date);
  const bool accepted = !data.empty();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k400BadRequest, service_log::MSG_GET_ERROR);
  if (accepted) {
    response = http::Response::ok(data, service_log::MSG_GET_OK);
  }
  callback(response);
}

void LogController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const std::string page = request->getParameter(service_log::QUERY_PAGE);
  const Json::Value data = BaseLogger::remove(page);
  const bool accepted = !data.empty();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k400BadRequest, service_log::MSG_REMOVE_ERROR);
  if (accepted) {
    response = http::Response::ok(data, service_log::MSG_REMOVE_OK);
  }
  logger.record(request, response, started);
  callback(response);
}

void LogController::enable(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  BaseLogger::setEnabled(true);
  Json::Value data(Json::objectValue);
  data[service_log::KEY_ENABLED] = true;
  const auto response = http::Response::ok(data, service_log::MSG_ENABLE_OK);
  logger.record(request, response, started);
  callback(response);
}

void LogController::disable(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  Json::Value data(Json::objectValue);
  data[service_log::KEY_ENABLED] = false;
  const auto response = http::Response::ok(data, service_log::MSG_DISABLE_OK);
  logger.record(request, response, started);
  callback(response);
  BaseLogger::setEnabled(false);
}

void LogController::status(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  Json::Value data(Json::objectValue);
  data[service_log::KEY_ENABLED] = BaseLogger::enabledValue();
  const auto response = http::Response::ok(data, service_log::MSG_STATUS_OK);
  logger.record(request, response, started);
  callback(response);
}
