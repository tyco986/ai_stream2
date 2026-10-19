#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class UserGroupService {
 public:
  explicit UserGroupService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& search, int page, int pageSize);
  Json::Value get(const std::string& groupId);
  Json::Value map();
  Json::Value create(const Json::Value& name,
                     const Json::Value& streamCrud,
                     const Json::Value& modelCrud,
                     const Json::Value& previewCrud,
                     const Json::Value& pipelineCrud,
                     const Json::Value& recordingCrud,
                     const Json::Value& serverCrud,
                     const Json::Value& userCrud,
                     const Json::Value& eventCrud);
  Json::Value update(const std::string& groupId,
                     const Json::Value& name,
                     const Json::Value& streamCrud,
                     const Json::Value& modelCrud,
                     const Json::Value& previewCrud,
                     const Json::Value& pipelineCrud,
                     const Json::Value& recordingCrud,
                     const Json::Value& serverCrud,
                     const Json::Value& userCrud,
                     const Json::Value& eventCrud);
  Json::Value members(const std::string& groupId);
  Json::Value candidates(const std::string& groupId);
  Json::Value setMembers(const std::string& groupId, const Json::Value& userIds);
  Json::Value remove(const std::string& groupId);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
