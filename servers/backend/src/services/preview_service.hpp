#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>
#include <unordered_map>
#include <vector>

class PreviewService {
 public:
  explicit PreviewService(drogon::orm::DbClientPtr dbClient);
  std::string token2UserId(const std::string& token);
  Json::Value get(const std::string& presetId, const std::string& userId);
  Json::Value map(const std::string& userId);
  Json::Value tree();
  Json::Value create(const std::string& userId,
                     const Json::Value& name,
                     const Json::Value& layout,
                     const Json::Value& mode,
                     const Json::Value& slots,
                     const Json::Value& active);
  Json::Value update(const std::string& presetId,
                     const std::string& userId,
                     const Json::Value& name,
                     const Json::Value& layout,
                     const Json::Value& mode,
                     const Json::Value& slots,
                     const Json::Value& active);
  Json::Value remove(const std::string& presetId, const std::string& userId);
  Json::Value getActive(const std::string& userId);

 private:
  std::string slots2Text(const std::string& layout, const Json::Value& slots);
  Json::Value parseSlots(Json::Value data);
  Json::Value row2json(const drogon::orm::Row& row);
  Json::Value groupNode(const Json::Value& group,
                        const std::unordered_map<std::string, std::vector<Json::Value>>& children,
                        const std::unordered_map<std::string, std::vector<Json::Value>>& streams);

  drogon::orm::DbClientPtr dbClient;
};
