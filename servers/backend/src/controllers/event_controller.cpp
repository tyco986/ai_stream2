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
    const auto response = http::Response::ok(
        service.list(request->getParameter(event::QUERY_SEARCH), request->getParameter(event::QUERY_LEVEL), page, pageSize),
        event::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, event::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void EventController::read(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const std::string& event_id) {
  setRead(request, std::move(callback), event_id, true, event::MSG_READ_OK, event::MSG_READ_ERROR);
}

void EventController::unread(const drogon::HttpRequestPtr& request,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const std::string& event_id) {
  setRead(request, std::move(callback), event_id, false, event::MSG_UNREAD_OK, event::MSG_UNREAD_ERROR);
}

void EventController::remove(const drogon::HttpRequestPtr& request,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                             const std::string& event_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.remove(event_id), event::MSG_REMOVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, event::MSG_REMOVE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void EventController::setRead(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& eventId,
                              bool read,
                              const char* ok,
                              const char* error) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.setRead(eventId, read), ok);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& failure) {
    const auto response = http::Response::error(drogon::k500InternalServerError, error);
    logger.record(request, response, started, failure.what());
    callback(response);
  }
}
