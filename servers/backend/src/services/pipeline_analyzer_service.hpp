#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class PipelineAnalyzerService {
 public:
  explicit PipelineAnalyzerService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& search, int page, int pageSize);
  Json::Value get(const std::string& analyzerId);
  Json::Value create(const Json::Value& name,
                     const Json::Value& annotations,
                     const std::string& fileName,
                     const std::string& fileContent);
  Json::Value update(const std::string& analyzerId,
                     const Json::Value& name,
                     const Json::Value& annotations,
                     const std::string& fileName,
                     const std::string& fileContent);
  Json::Value remove(const std::string& analyzerId);

 private:
  std::string saveFile(const std::string& analyzerId, const std::string& fileName, const std::string& fileContent);
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
