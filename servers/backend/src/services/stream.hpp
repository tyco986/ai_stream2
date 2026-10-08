#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>
#include <vector>

class StreamService {
 public:
  explicit StreamService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& groupId,
                   const std::string& streamId,
                   const std::string& search,
                   int page,
                   int pageSize);
  Json::Value create(drogon::orm::DbClient& tx,
                     const std::string& name,
                     const std::string& groupId,
                     const std::string& url,
                     bool enabled,
                     bool recording);
  Json::Value get(const std::string& streamId);
  Json::Value update(drogon::orm::DbClient& tx,
                     const std::string& streamId,
                     const std::string& name,
                     const std::string& groupId,
                     const std::string& url,
                     bool enabled,
                     bool recording);
  void enable(drogon::orm::DbClient& tx, const std::string& streamId);
  void disable(drogon::orm::DbClient& tx, const std::string& streamId);
  void record(drogon::orm::DbClient& tx, const std::string& streamId);
  void unrecord(drogon::orm::DbClient& tx, const std::string& streamId);
  void remove(const std::string& streamId);
  void batchRemove(const std::vector<std::string>& streamIds);
  Json::Value probe(const std::string& streamId, const Json::Value& result);

 private:
  Json::Value item(const drogon::orm::Row& row);
  Json::Value probeItem(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
