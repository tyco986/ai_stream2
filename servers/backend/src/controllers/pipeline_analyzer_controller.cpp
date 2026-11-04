#include "controllers/pipeline_analyzer_controller.hpp"

#include "db/database.hpp"
#include "logger/base_logger.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

PipelineAnalyzerController::PipelineAnalyzerController() : service(Database::client()) {}

void PipelineAnalyzerController::list(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(pipeline_analyzer::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(pipeline_analyzer::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? pipeline_analyzer::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? pipeline_analyzer::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    logger.respond(request,
            http::Response::ok(service.list(request->getParameter(pipeline_analyzer::QUERY_SEARCH), page, pageSize),
                               pipeline_analyzer::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& analyzer_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request, http::Response::ok(service.get(analyzer_id), pipeline_analyzer::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::create(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    logger.respond(request,
            http::Response::ok(service.create(payload[pipeline_analyzer::KEY_NAME],
                                              payload[pipeline_analyzer::KEY_ANNOTATIONS],
                                              payload[pipeline_analyzer::KEY_MEDIA_ID]),
                               pipeline_analyzer::MSG_CREATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_CREATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::update(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& analyzer_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    logger.respond(request,
            http::Response::ok(service.update(analyzer_id,
                                              payload[pipeline_analyzer::KEY_NAME],
                                              payload[pipeline_analyzer::KEY_ANNOTATIONS],
                                              payload[pipeline_analyzer::KEY_MEDIA_ID]),
                               pipeline_analyzer::MSG_UPDATE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, pipeline_analyzer::MSG_UPDATE_ERROR),
            started,
            callback,
            error.what());
  }
}

void PipelineAnalyzerController::remove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& analyzer_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request, http::Response::ok(service.remove(analyzer_id), pipeline_analyzer::MSG_REMOVE_OK), started, callback);
  } catch (const std::exception& error) {
    const bool occupied = std::string(error.what()) == pipeline_analyzer::ERR_OCCUPIED;
    logger.respond(request,
            http::Response::error(occupied ? drogon::k409Conflict : drogon::k500InternalServerError,
                                  occupied ? pipeline_analyzer::ERR_OCCUPIED : pipeline_analyzer::MSG_REMOVE_ERROR),
            started,
            callback,
            error.what());
  }
}
