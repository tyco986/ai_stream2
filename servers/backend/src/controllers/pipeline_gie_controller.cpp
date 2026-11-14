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
    const auto response = http::Response::ok(service.list(request->getParameter(pipeline_gie::QUERY_SEARCH), page, pageSize),
                               pipeline_gie::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PipelineGieController::getModel(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.getModel(), pipeline_gie::MSG_MODELS_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_MODELS_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PipelineGieController::map(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.map(), pipeline_gie::MSG_MAP_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_MAP_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PipelineGieController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& gie_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.get(gie_id), pipeline_gie::MSG_GET_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_GET_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PipelineGieController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    const auto response = http::Response::ok(service.create(payload[pipeline_gie::KEY_MODEL_ID],
                                              payload[pipeline_gie::KEY_NAME],
                                              payload[pipeline_gie::KEY_CLASS_ATTRS],
                                              payload[pipeline_gie::KEY_INTERVAL]),
                               pipeline_gie::MSG_CREATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_CREATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
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
    const auto response = http::Response::ok(service.update(gie_id,
                                              payload[pipeline_gie::KEY_MODEL_ID],
                                              payload[pipeline_gie::KEY_NAME],
                                              payload[pipeline_gie::KEY_CLASS_ATTRS],
                                              payload[pipeline_gie::KEY_INTERVAL]),
                               pipeline_gie::MSG_UPDATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, pipeline_gie::MSG_UPDATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void PipelineGieController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& gie_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.remove(gie_id), pipeline_gie::MSG_REMOVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const bool occupied = std::string(error.what()) == pipeline_gie::ERR_OCCUPIED;
    const auto response = http::Response::error(occupied ? drogon::k409Conflict : drogon::k500InternalServerError,
                                  occupied ? pipeline_gie::ERR_OCCUPIED : pipeline_gie::MSG_REMOVE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}
