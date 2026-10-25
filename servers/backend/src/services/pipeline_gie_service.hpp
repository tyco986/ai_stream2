#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class PipelineGieService {
 public:
  explicit PipelineGieService(drogon::orm::DbClientPtr dbClient);
  Json::Value getModel();
  Json::Value map();
  Json::Value list(const std::string& search, int page, long pageSize);
  Json::Value get(const std::string& gieId);
  Json::Value create(const Json::Value& modelId,
                     const Json::Value& name,
                     const Json::Value& classAttrs,
                     const Json::Value& interval);
  Json::Value update(const std::string& gieId,
                     const Json::Value& modelId,
                     const Json::Value& name,
                     const Json::Value& classAttrs,
                     const Json::Value& interval);
  Json::Value remove(const std::string& gieId);

 private:
  Json::Value row2json(const drogon::orm::Row& row, const std::string& enginePath);

  drogon::orm::DbClientPtr dbClient;
};
