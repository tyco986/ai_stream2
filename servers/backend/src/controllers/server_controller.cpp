#include "controllers/server_controller.hpp"

#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>

ServerController::ServerController() : service(Database::client()) {}

void ServerController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.get(), server::MSG_GET_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, server::MSG_GET_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

drogon::Task<drogon::HttpResponsePtr> ServerController::probe(drogon::HttpRequestPtr request) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, server::MSG_PROBE_ERROR);
  std::string errorText;
  try {
    response = http::Response::ok(co_await service.probe(), server::MSG_PROBE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, server::MSG_PROBE_ERROR);
    errorText = error.what();
  }
  logger.record(request, response, started, errorText);
  co_return response;
}
