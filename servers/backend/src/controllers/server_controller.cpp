#include "controllers/server_controller.hpp"

#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>

ServerController::ServerController() : service(Database::client()) {}

void ServerController::respond(
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

void ServerController::get(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    respond(request, http::Response::ok(service.get(), server::MSG_GET_OK), started, callback);
  } catch (const std::exception& error) {
    respond(request,
            http::Response::error(drogon::k500InternalServerError, server::MSG_GET_ERROR),
            started,
            callback,
            error.what());
  }
}

drogon::Task<drogon::HttpResponsePtr> ServerController::probe(const drogon::HttpRequestPtr& request) {
  const auto started = std::chrono::steady_clock::now();
  drogon::HttpResponsePtr response = http::Response::error(drogon::k500InternalServerError, server::MSG_PROBE_ERROR);
  std::string errorText;
  try {
    response = http::Response::ok(co_await service.probe(), server::MSG_PROBE_OK);
  } catch (const std::exception& error) {
    response = http::Response::error(drogon::k500InternalServerError, server::MSG_PROBE_ERROR);
    errorText = error.what();
  }
  respond(request, response, started, [](const drogon::HttpResponsePtr&) {}, errorText);
  co_return response;
}
