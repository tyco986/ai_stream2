#pragma once

#include "constants/notice_const.hpp"
#include "log/notice_logger.hpp"
#include "services/notice_service.hpp"

#include <drogon/HttpController.h>

#include <chrono>
#include <functional>
#include <string>

class NoticeController : public drogon::HttpController<NoticeController> {
 public:
  NoticeController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(NoticeController::list, notice::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(NoticeController::unread, notice::PATH_UNREAD, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(NoticeController::setReadAt, notice::PATH_READ_AT, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(NoticeController::remove, notice::PATH_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void unread(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void setReadAt(const drogon::HttpRequestPtr& request,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 const std::string& notice_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& notice_id);

 private:
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

  NoticeService service;
  NoticeLogger logger;
};
