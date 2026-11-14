#include "controllers/notice_controller.hpp"

#include "constants/notice_const.hpp"
#include "db/database.hpp"
#include "response/response.hpp"

#include <exception>
#include <string>

NoticeController::NoticeController() : service(Database::client()) {}

void NoticeController::list(const drogon::HttpRequestPtr& request,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto pageText = request->getParameter(notice::QUERY_PAGE);
    const auto pageSizeText = request->getParameter(notice::QUERY_PAGE_SIZE);
    const int page = pageText.empty() ? notice::DEFAULT_PAGE : std::stoi(pageText);
    const int pageSize = pageSizeText.empty() ? notice::DEFAULT_PAGE_SIZE : std::stoi(pageSizeText);
    const std::string userId = service.token2UserId(request->getHeader("authorization"));
    const auto response = http::Response::ok(service.list(userId, page, pageSize), notice::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void NoticeController::unread(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const std::string userId = service.token2UserId(request->getHeader("authorization"));
    Json::Value data;
    data[notice::KEY_TOTAL] = static_cast<Json::Int64>(service.countUnread(userId));
    const auto response = http::Response::ok(data, notice::MSG_UNREAD_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_UNREAD_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void NoticeController::setReadAt(const drogon::HttpRequestPtr& request,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const std::string& notice_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    const auto response = http::Response::ok(service.setReadAt(notice_id, payload[notice::KEY_READ_AT].asString()),
                               notice::MSG_READ_AT_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_READ_AT_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void NoticeController::remove(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& notice_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.remove(notice_id), notice::MSG_REMOVE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_REMOVE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}
