#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class UserService {
 public:
  explicit UserService(drogon::orm::DbClientPtr dbClient);
  Json::Value login(const Json::Value& username, const Json::Value& passwd);
  Json::Value list(const std::string& search, int page, int pageSize);
  Json::Value get(const std::string& userId);
  Json::Value map();
  Json::Value create(const Json::Value& username,
                     const Json::Value& passwd,
                     const Json::Value& active,
                     const Json::Value& groupId);
  Json::Value update(const std::string& userId,
                     const Json::Value& username,
                     const Json::Value& passwd,
                     const Json::Value& active,
                     const Json::Value& groupId);
  Json::Value remove(const std::string& userId);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
