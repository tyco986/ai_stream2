#include "controllers/event_controller.hpp"

#include "constants/event_const.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

EventController::EventController() : service(Database::client()) {}

void EventController::list(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(event::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(event::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? event::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? event::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    const auto response = http::Response::ok(service.list(request->getParameter(event::QUERY_SEARCH), page, pageSize),
                               event::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, event::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void EventController::setStatus(const drogon::HttpRequestPtr& request,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const std::string& event_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    const auto response = http::Response::ok(service.setStatus(event_id, payload[event::KEY_STATUS].asString()),
                               event::MSG_STATUS_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, event::MSG_STATUS_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}
