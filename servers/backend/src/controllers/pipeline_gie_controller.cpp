#include "controllers/pipeline_gie_controller.hpp"

#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

PipelineGieController::PipelineGieController() : service(Database::client()) {}

void PipelineGieController::respond(
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

void PipelineGieController::getModel(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.getModel(), pipeline_gie::MSG_MODELS_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_MODELS_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineGieController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.map(), pipeline_gie::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_MAP_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineGieController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& gie_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.get(gie_id), pipeline_gie::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineGieController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    respond(request,
            http::Response::ok(service.create(payload[pipeline_gie::KEY_MODEL_ID],
                                              payload[pipeline_gie::KEY_NAME],
                                              payload[pipeline_gie::KEY_CLASS_ATTRS],
                                              payload[pipeline_gie::KEY_INTERVAL]),
                               pipeline_gie::MSG_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_CREATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineGieController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& gie_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    respond(request,
            http::Response::ok(service.update(gie_id,
                                              payload[pipeline_gie::KEY_MODEL_ID],
                                              payload[pipeline_gie::KEY_NAME],
                                              payload[pipeline_gie::KEY_CLASS_ATTRS],
                                              payload[pipeline_gie::KEY_INTERVAL]),
                               pipeline_gie::MSG_UPDATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_UPDATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineGieController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& gie_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.remove(gie_id), pipeline_gie::MSG_REMOVE_OK), started, callback);
  } catch (const std::exception& error) {
    const bool occupied = std::string(error.what()) == pipeline_gie::ERR_OCCUPIED;
    respond(request,
            http::Response::error(occupied ? drogon::k409Conflict : drogon::k500InternalServerError,
                                  occupied ? pipeline_gie::ERR_OCCUPIED : pipeline_gie::MSG_REMOVE_ERROR),
            started,
            callback,
            error.what());
  }
}
