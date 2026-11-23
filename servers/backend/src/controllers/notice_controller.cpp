#include "controllers/notice_controller.hpp"

#include "auth/auth.hpp"
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
    const auto response = http::Response::ok(
        service.list(request->getParameter(notice::QUERY_LEVEL), page, pageSize), notice::MSG_LIST_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_LIST_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void NoticeController::create(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  const auto body = request->getJsonObject();
  const Json::Value payload = body ? *body : Json::Value(Json::objectValue);
  try {
    const auto response = http::Response::ok(service.create(payload[notice::KEY_CREATED_AT],
                                              payload[notice::KEY_LEVEL],
                                              payload[notice::KEY_SOURCE],
                                              payload[notice::KEY_TEXT],
                                              payload[notice::KEY_REF_ID]),
                               notice::MSG_CREATE_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_CREATE_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void NoticeController::count(const drogon::HttpRequestPtr& request,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.count(), notice::MSG_COUNT_OK);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& error) {
    const auto response = http::Response::error(drogon::k500InternalServerError, notice::MSG_COUNT_ERROR);
    logger.record(request, response, started, error.what());
    callback(response);
  }
}

void NoticeController::read(const drogon::HttpRequestPtr& request,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            const std::string& notice_id) {
  setRead(request, std::move(callback), notice_id, true, notice::MSG_READ_OK, notice::MSG_READ_ERROR);
}

void NoticeController::unread(const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              const std::string& notice_id) {
  setRead(request, std::move(callback), notice_id, false, notice::MSG_UNREAD_OK, notice::MSG_UNREAD_ERROR);
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

void NoticeController::setRead(const drogon::HttpRequestPtr& request,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               const std::string& noticeId,
                               bool read,
                               const char* ok,
                               const char* error) {
  const auto started = std::chrono::steady_clock::now();
  try {
    const auto response = http::Response::ok(service.setRead(noticeId, read, service.token2UserId(AuthFilter::token(request))), ok);
    logger.record(request, response, started);
    callback(response);
  } catch (const std::exception& failure) {
    const auto response = http::Response::error(drogon::k500InternalServerError, error);
    logger.record(request, response, started, failure.what());
    callback(response);
  }
}
