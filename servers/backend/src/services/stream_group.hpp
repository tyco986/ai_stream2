#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class StreamGroupService {
 public:
  explicit StreamGroupService(drogon::orm::DbClientPtr dbClient);
  Json::Value tree();
  Json::Value map();
  Json::Value rename(const std::string& groupId, const std::string& name);
  Json::Value remove(const std::string& groupId);
  Json::Value create(const std::string& parentGroupId, const std::string& name);
  Json::Value members(const std::string& groupId);
  Json::Value candidates(const std::string& groupId);
  Json::Value setMembers(const std::string& groupId, const std::vector<std::string>& streamIds);

 private:
  struct GroupRow {
    std::string id;
    std::string name;
    std::string parentId;
  };

  struct StreamRow {
    std::string id;
    std::string name;
    bool enabled;
    std::string status;
    bool recording;
  };

  std::vector<std::string> subtreeIds(const std::string& groupId);

  Json::Value groupNode(const GroupRow& group,
                        const std::unordered_map<std::string, std::vector<GroupRow>>& children,
                        const std::unordered_map<std::string, std::vector<StreamRow>>& streams);
  Json::Value streamNode(const StreamRow& row);

  drogon::orm::DbClientPtr dbClient;
};
