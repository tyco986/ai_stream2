#pragma once

#include <drogon/HttpClient.h>
#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class ServerService {
 public:
  explicit ServerService(drogon::orm::DbClientPtr dbClient);
  Json::Value get();
  drogon::Task<Json::Value> probe();

 private:
  drogon::Task<bool> healthy(const std::string& baseUrl, const std::string& path);
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
