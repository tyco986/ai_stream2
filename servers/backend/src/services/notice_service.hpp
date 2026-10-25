#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class NoticeService {
 public:
  explicit NoticeService(drogon::orm::DbClientPtr dbClient);
  Json::Value create(const Json::Value& createdAt,
                     const Json::Value& userId,
                     const Json::Value& level,
                     const Json::Value& source,
                     const Json::Value& text,
                     const Json::Value& refId);
  std::string token2UserId(const std::string& token);
  Json::Value list(const std::string& userId, int page, long pageSize);
  Json::Value remove(const std::string& noticeId);
  long countUnread(const std::string& userId);
  Json::Value setReadAt(const std::string& noticeId, const std::string& readAt);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
