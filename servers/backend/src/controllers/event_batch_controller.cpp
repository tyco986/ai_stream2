#include "controllers/event_batch_controller.hpp"

#include "auth/auth.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

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
  const bool hasIds = body && (*body).isMember(event::KEY_IDS) && (*body)[event::KEY_IDS].isArray();
  const Json::Value ids = hasIds ? (*body)[event::KEY_IDS] : Json::Value(Json::arrayValue);
  const Json::Value data = remove ? service.batchRemove(ids)
                                  : service.batchSetRead(ids, read, service.token2UserId(AuthFilter::token(request)));
  const auto response = http::Response::ok(data, message);
  logger.record(request, response, started);
  callback(response);
}
