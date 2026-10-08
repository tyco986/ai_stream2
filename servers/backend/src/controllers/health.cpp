#include "controllers/health.hpp"

#include <drogon/HttpResponse.h>

void HealthController::health(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  auto response = drogon::HttpResponse::newHttpResponse();
  callback(std::move(response));
}
