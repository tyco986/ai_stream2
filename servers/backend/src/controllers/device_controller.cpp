#include "controllers/device_controller.hpp"

#include "response/response.hpp"

#include <exception>

void DeviceController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.list(), device::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, device::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}
