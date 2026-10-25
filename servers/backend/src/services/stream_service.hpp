#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class StreamService {
 public:
  explicit StreamService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& groupId,
                   const std::string& streamId,
                   const std::string& search,
                   int page,
                   long pageSize);
  Json::Value create(const Json::Value& name,
                     const Json::Value& groupId,
                     const Json::Value& url,
                     const Json::Value& mount,
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
                     const Json::Value& mount,
                     const Json::Value& recording,
                     const Json::Value& status,
                     const Json::Value& width,
                     const Json::Value& height,
                     const Json::Value& fps,
                     const Json::Value& probedAt);
  void setMount(const std::string& streamId, bool mount);
  void setRecord(const std::string& streamId, bool recording);
  Json::Value remove(const std::string& streamId);
  Json::Value importCSV(const std::string& csv);
  std::string exportCSV();
  Json::Value probe(const std::string& streamId,
                    const Json::Value& status,
                    const Json::Value& width,
                    const Json::Value& height,
                    const Json::Value& fps,
                    const Json::Value& probedAt);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
