#include "controllers/user_batch_controller.hpp"

#include "db/database.hpp"
#include "log/base_logger.hpp"
#include "response/response.hpp"

#include <chrono>
#include <exception>
#include <string>

UserBatchController::UserBatchController() : service(Database::client()) {}

void UserBatchController::respond(
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

void UserBatchController::batchRemove(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value data = user::BATCH_DATA();
  const bool hasIds = body && (*body).isMember(user::KEY_IDS) && (*body)[user::KEY_IDS].isArray();
  if (hasIds) {
    for (const auto& node : (*body)[user::KEY_IDS]) {
      try {
        data[user::KEY_UPDATED_ROWS].append(service.remove(node.asString()));
      } catch (const std::exception&) {
        Json::Value failed;
        failed[user::KEY_ID] = node.asString();
        data[user::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[user::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[user::KEY_UPDATED_ROWS].size());
  data[user::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[user::KEY_FAILED_ROWS].size());
  data[user::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[user::KEY_SKIPPED_ROWS].size());
  respond(request, http::Response::ok(data, user::MSG_BATCH_REMOVE_OK), started, callback);
}
