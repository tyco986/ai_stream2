#include "controllers/event_controller.hpp"

#include "auth/auth.hpp"
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
    const auto pageText = request->getParameter(event::KEY_PAGE);
    const auto pageSizeText = request->getParameter(event::KEY_PAGE_SIZE);
    const int page = pageText.empty() ? event::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? event::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    const auto response = http::Response::ok(
        service.list(request->getParameter(event::QUERY_SEARCH), request->getParameter(event::KEY_LEVEL), page, pageSize),
        event::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, event::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void EventController::content(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& event_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = drogon::HttpResponse::newFileResponse(service.contentPath(event_id, request->getParameter(event::QUERY_KIND)));
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, event::MSG_CONTENT_ERROR);
    errorText = error.what();
  }
  logger.record(request, response, started, errorText);
  callback(response);
}

void EventController::contentInfo(const drogon::HttpRequestPtr& request,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  const std::string& event_id) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response;
  std::string errorText;
  try {
    response = http::Response::ok(service.contentInfo(event_id, request->getParameter(event::QUERY_KIND)), event::MSG_CONTENT_INFO_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, event::MSG_CONTENT_INFO_ERROR);
    errorText = error.what();
  }
  logger.record(request, response, started, errorText);
  callback(response);
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
    const auto response = http::Response::ok(service.setRead(eventId, read, service.token2UserId(AuthFilter::token(request))), ok);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& failure) {
    const auto response = http::Response::error(drogon::k500InternalServerError, error);
    logger.record(request, response, started, failure.what());
    callback(response);
  }
}
