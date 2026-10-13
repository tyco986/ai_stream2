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
  Json::Value create(const Json::Value& name,
                     const Json::Value& groupId,
                     const Json::Value& url,
                     const Json::Value& enabled,
                     const Json::Value& recording,
                     const Json::Value& status,
                     const Json::Value& width,
                     const Json::Value& height,
                     const Json::Value& fps,
                     const Json::Value& probedAt);
  Json::Value get(const std::string& streamId);
  Json::Value update(const std::string& streamId,
                     const Json::Value& name,
                     const Json::Value& groupId,
                     const Json::Value& url,
                     const Json::Value& enabled,
                     const Json::Value& recording,
                     const Json::Value& status,
                     const Json::Value& width,
                     const Json::Value& height,
                     const Json::Value& fps,
                     const Json::Value& probedAt);
  void enable(const std::string& streamId);
  void disable(const std::string& streamId);
  void record(const std::string& streamId);
  void unrecord(const std::string& streamId);
  void remove(const std::string& streamId);
  void batchRemove(const std::vector<std::string>& streamIds);
  Json::Value importCSV(const std::string& csv);
  std::string exportCSV();
  Json::Value probe(const std::string& streamId,
                    const Json::Value& status,
                    const Json::Value& width,
                    const Json::Value& height,
                    const Json::Value& fps,
                    const Json::Value& probedAt);

 private:
  Json::Value item(const drogon::orm::Row& row);
  Json::Value probeItem(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
