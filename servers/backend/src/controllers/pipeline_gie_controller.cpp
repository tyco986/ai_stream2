#include "controllers/pipeline_gie_controller.hpp"

#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

PipelineGieController::PipelineGieController() : service(Database::client()) {}

void PipelineGieController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(pipeline_gie::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(pipeline_gie::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? pipeline_gie::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? pipeline_gie::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    logger.respond(request,
            http::Response::ok(service.list(request->getParameter(pipeline_gie::QUERY_SEARCH), page, pageSize),
                               pipeline_gie::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineGieController::getModel(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request, http::Response::ok(service.getModel(), pipeline_gie::MSG_MODELS_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request, http::Response::ok(service.map(), pipeline_gie::MSG_MAP_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request, http::Response::ok(service.get(gie_id), pipeline_gie::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request,
            http::Response::ok(service.create(payload[pipeline_gie::KEY_MODEL_ID],
                                              payload[pipeline_gie::KEY_NAME],
                                              payload[pipeline_gie::KEY_CLASS_ATTRS],
                                              payload[pipeline_gie::KEY_INTERVAL]),
                               pipeline_gie::MSG_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request,
            http::Response::ok(service.update(gie_id,
                                              payload[pipeline_gie::KEY_MODEL_ID],
                                              payload[pipeline_gie::KEY_NAME],
                                              payload[pipeline_gie::KEY_CLASS_ATTRS],
                                              payload[pipeline_gie::KEY_INTERVAL]),
                               pipeline_gie::MSG_UPDATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
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
    logger.respond(request, http::Response::ok(service.remove(gie_id), pipeline_gie::MSG_REMOVE_OK), started, callback);
  } catch (const std::exception& error) {
    const bool occupied = std::string(error.what()) == pipeline_gie::ERR_OCCUPIED;
    logger.respond(request,
            http::Response::error(occupied ? drogon::k409Conflict : drogon::k500InternalServerError,
                                  occupied ? pipeline_gie::ERR_OCCUPIED : pipeline_gie::MSG_REMOVE_ERROR),
            started,
            callback,
            error.what());
  }
}
