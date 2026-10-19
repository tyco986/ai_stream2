#include "controllers/preview_controller.hpp"

#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <exception>

PreviewController::PreviewController() : service(Database::client()) {}

void PreviewController::respond(
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

void PreviewController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.map(service.token2UserId(request->getHeader("authorization"))), preview::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_MAP_ERROR),
            started,
            callback,
            error.what());
  }
}

void PreviewController::tree(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.tree(), preview::MSG_TREE_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_TREE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PreviewController::getActive(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request,
            http::Response::ok(service.getActive(service.token2UserId(request->getHeader("authorization"))), preview::MSG_ACTIVE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_ACTIVE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PreviewController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& preset_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request,
            http::Response::ok(service.get(preset_id, service.token2UserId(request->getHeader("authorization"))), preview::MSG_GET_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_GET_ERROR),
            started,
            callback,
            error.what());
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
    respond(request,
            http::Response::ok(service.create(service.token2UserId(request->getHeader("authorization")),
                                              json[preview::KEY_NAME],
                                              json[preview::KEY_LAYOUT],
                                              json[preview::KEY_MODE],
                                              json[preview::KEY_SLOTS],
                                              json[preview::KEY_ACTIVE]),
                               preview::MSG_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_CREATE_ERROR),
            started,
            callback,
            error.what());
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
    respond(request,
            http::Response::ok(service.update(preset_id,
                                              service.token2UserId(request->getHeader("authorization")),
                                              json[preview::KEY_NAME],
                                              json[preview::KEY_LAYOUT],
                                              json[preview::KEY_MODE],
                                              json[preview::KEY_SLOTS],
                                              json[preview::KEY_ACTIVE]),
                               preview::MSG_UPDATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_UPDATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PreviewController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& preset_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request,
            http::Response::ok(service.remove(preset_id, service.token2UserId(request->getHeader("authorization"))), preview::MSG_REMOVE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, preview::MSG_REMOVE_ERROR),
            started,
            callback,
            error.what());
  }
}
