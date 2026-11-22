#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class NoticeService {
 public:
  explicit NoticeService(drogon::orm::DbClientPtr dbClient);
  Json::Value create(const Json::Value& createdAt,
                     const Json::Value& level,
                     const Json::Value& source,
                     const Json::Value& text,
                     const Json::Value& refId);
  Json::Value list(const std::string& level, int page, long pageSize);
  Json::Value remove(const std::string& noticeId);
  Json::Value count();
  Json::Value setRead(const std::string& noticeId, bool read);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
