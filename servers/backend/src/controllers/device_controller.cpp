#include "controllers/device_controller.hpp"

#include "response/response.hpp"

#include <exception>

void DeviceController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request, http::Response::ok(service.list(), device::MSG_LIST_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
                   http::Response::error(drogon::k500InternalServerError, device::MSG_LIST_ERROR),
                   started,
                   callback,
                   error.what());
  }
}
