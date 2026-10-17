#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>
#include <yaml-cpp/yaml.h>

#include <string>

class PipelineService {
 public:
  explicit PipelineService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& search, int page, int pageSize);
  Json::Value map();
  Json::Value get(const std::string& pipelineId);
  std::string createStart(const Json::Value& input);
  Json::Value createEnd(const Json::Value& status,
                        const Json::Value& configPath,
                        const Json::Value& name,
                        const Json::Value& type,
                        const Json::Value& analyzerIds,
                        const Json::Value& streamIds,
                        const Json::Value& gieIds,
                        const Json::Value& input);
  Json::Value update(const std::string& pipelineId,
                        const Json::Value& status,
                        const Json::Value& configPath,
                        const Json::Value& name,
                        const Json::Value& type,
                        const Json::Value& analyzerIds,
                        const Json::Value& gieIds,
                        const Json::Value& input);
  Json::Value remove(const std::string& pipelineId);
  Json::Value setStatus(const std::string& pipelineId, const std::string& status);
  Json::Value probe(const std::string& pipelineId,
                    const std::string& status,
                    const Json::Value& startTime,
                    const Json::Value& endTime,
                    const Json::Value& duration);

 private:
  YAML::Node json2yaml(const Json::Value& value);
  Json::Value json2array(const std::string& text);
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
