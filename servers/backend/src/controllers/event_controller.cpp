#include "controllers/event_controller.hpp"

#include "constants/event_const.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

EventController::EventController() : service(Database::client()) {}

void EventController::respond(const drogon::HttpRequestPtr& request,
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

void EventController::list(const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(event::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(event::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? event::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? event::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    respond(request,
            http::Response::ok(service.list(request->getParameter(event::QUERY_SEARCH), page, pageSize),
                               event::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, event::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void EventController::setStatus(const drogon::HttpRequestPtr& request,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                const std::string& event_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    respond(request,
            http::Response::ok(service.setStatus(event_id, payload[event::KEY_STATUS].asString()),
                               event::MSG_STATUS_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, event::MSG_STATUS_ERROR),
            started,
            callback,
            error.what());
  }
}
