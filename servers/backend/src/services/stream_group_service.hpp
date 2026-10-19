#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>
#include <unordered_map>
#include <vector>

class StreamGroupService {
 public:
  explicit StreamGroupService(drogon::orm::DbClientPtr dbClient);
  Json::Value tree();
  Json::Value map();
  Json::Value rename(const std::string& groupId, const Json::Value& name);
  Json::Value remove(const std::string& groupId);
  Json::Value create(const std::string& parentGroupId, const Json::Value& name);
  Json::Value members(const std::string& groupId);
  Json::Value candidates(const std::string& groupId);
  Json::Value setMembers(const std::string& groupId, const Json::Value& streamIds);
  Json::Value importCSV(const std::string& csv);
  std::string exportCSV();

 private:
  std::vector<std::string> subtreeIds(const std::string& groupId);

  Json::Value groupNode(const Json::Value& group,
                        const std::unordered_map<std::string, std::vector<Json::Value>>& children,
                        const std::unordered_map<std::string, std::vector<Json::Value>>& streams);

  drogon::orm::DbClientPtr dbClient;
};
