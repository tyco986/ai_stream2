#pragma once

#include "constants/notice_const.hpp"
#include "logger/notice_logger.hpp"
#include "services/notice_service.hpp"

#include <drogon/HttpController.h>

#include <functional>
#include <string>

class NoticeController : public drogon::HttpController<NoticeController> {
 public:
  NoticeController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(NoticeController::list, notice::PATH_LIST, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(NoticeController::create, notice::PATH_LIST, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(NoticeController::count, notice::PATH_COUNT, drogon::Get, "AuthFilter");
  ADD_METHOD_TO(NoticeController::read, notice::PATH_READ, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(NoticeController::unread, notice::PATH_UNREAD, drogon::Post, "AuthFilter");
  ADD_METHOD_TO(NoticeController::remove, notice::PATH_ITEM, drogon::Delete, "AuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void create(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void count(const drogon::HttpRequestPtr& request, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
  void read(const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& notice_id);
  void unread(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& notice_id);
  void remove(const drogon::HttpRequestPtr& request,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
              const std::string& notice_id);

 private:
  void setRead(const drogon::HttpRequestPtr& request,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& noticeId,
               bool read,
               const char* ok,
               const char* error);

  NoticeService service;
  NoticeLogger logger;
};
