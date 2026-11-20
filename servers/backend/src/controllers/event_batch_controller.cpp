#include "controllers/event_batch_controller.hpp"

#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

EventBatchController::EventBatchController() : service(Database::client()) {}

void EventBatchController::batchRead(const drogon::HttpRequestPtr& request,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  apply(request, std::move(callback), false, true, event::MSG_BATCH_READ_OK);
}

void EventBatchController::batchUnread(const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  apply(request, std::move(callback), false, false, event::MSG_BATCH_UNREAD_OK);
}

void EventBatchController::batchRemove(const drogon::HttpRequestPtr& request,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  apply(request, std::move(callback), true, false, event::MSG_BATCH_REMOVE_OK);
}

void EventBatchController::apply(const drogon::HttpRequestPtr& request,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 bool remove,
                                 bool read,
                                 const char* message) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  Json::Value data = event::BATCH_DATA();
  const bool hasIds = body && (*body).isMember(event::KEY_IDS) && (*body)[event::KEY_IDS].isArray();
  if (hasIds) {
    for (const auto& node : (*body)[event::KEY_IDS]) {
      try {
        const Json::Value row = remove ? service.remove(node.asString()) : service.setRead(node.asString(), read);
        data[event::KEY_UPDATED_ROWS].append(row);
      } catch (const std::exception&) {
        Json::Value failed;
        failed[event::KEY_ID] = node.asString();
        data[event::KEY_FAILED_ROWS].append(failed);
      }
    }
  }
  data[event::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[event::KEY_UPDATED_ROWS].size());
  data[event::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[event::KEY_FAILED_ROWS].size());
  const auto response = http::Response::ok(data, message);
  logger.record(request, response, started);
  callback(response);
}
