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
    logger.respond(request,
            http::Response::ok(service.list(userId, page, pageSize), notice::MSG_LIST_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, notice::MSG_LIST_ERROR),
            started,
            callback,
            error.what());
  }
}

void NoticeController::unread(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const std::string userId = service.token2UserId(request->getHeader("authorization"));
    Json::Value data;
    data[notice::KEY_TOTAL] = static_cast<Json::Int64>(service.countUnread(userId));
    logger.respond(request, http::Response::ok(data, notice::MSG_UNREAD_OK), started, callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, notice::MSG_UNREAD_ERROR),
            started,
            callback,
            error.what());
  }
}

void NoticeController::setReadAt(const drogon::HttpRequestPtr& request,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 const std::string& notice_id) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    logger.respond(request,
            http::Response::ok(service.setReadAt(notice_id, payload[notice::KEY_READ_AT].asString()),
                               notice::MSG_READ_AT_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, notice::MSG_READ_AT_ERROR),
            started,
            callback,
            error.what());
  }
}

void NoticeController::remove(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& notice_id) {
  const auto started = std::chrono::steady_clock::now();
  try {
    logger.respond(request,
            http::Response::ok(service.remove(notice_id), notice::MSG_REMOVE_OK),
            started,
            callback);
  } catch (const std::exception& error) {
    logger.respond(request,
            http::Response::error(drogon::k500InternalServerError, notice::MSG_REMOVE_ERROR),
            started,
            callback,
            error.what());
  }
}
