#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

class PipelineService {
 public:
  explicit PipelineService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& search, int page, long pageSize);
  Json::Value map();
  Json::Value get(const std::string& pipelineId);
  bool queryDate(const std::string& date);
  bool queryPad(const std::string& pad);
  Json::Value getApiLogs(const std::string& name, const std::string& date);
  Json::Value getPadLogs(const std::string& name, const std::string& pad, const std::string& date);
  Json::Value getGstreamerLogs(const std::string& name, const std::string& date);
  std::string createStart(const Json::Value& input);
  Json::Value assembleInput(Json::Value input);
  Json::Value addOutput(Json::Value input, const std::string& name);
  Json::Value createEnd(const Json::Value& status,
                        const Json::Value& configPath,
                        const Json::Value& name,
                        const Json::Value& type,
                        const Json::Value& input);
  Json::Value setConfigPath(const std::string& pipelineId, const std::string& configPath);
  Json::Value update(const std::string& pipelineId,
                        const Json::Value& status,
                        const Json::Value& configPath,
                        const Json::Value& name,
                        const Json::Value& type,
                        const Json::Value& input);
  Json::Value remove(const std::string& pipelineId);
  Json::Value setStatus(const std::string& pipelineId, const std::string& status);
  Json::Value probe(const std::string& pipelineId,
                    const std::string& status,
                    const Json::Value& startTime,
                    const Json::Value& endTime,
                    const Json::Value& duration);
  YAML::Node readParams(const std::string& configPath);
  std::vector<std::string> getModelPaths(const YAML::Node& params);
  std::string packMedia(const YAML::Node& params);

 private:
  Json::Value readJsonl(std::istream& input);
  void collectModelZips(const YAML::Node& node, std::vector<std::string>& paths);
  void collectMedia(const YAML::Node& params, std::vector<std::string>& paths);
  YAML::Node json2yaml(const Json::Value& value);
  Json::Value json2array(const std::string& text);
  Json::Value row2json(const drogon::orm::Row& row);
  void expandGie(Json::Value& input);
  void expandStream(Json::Value& input, Json::Value& analyzer);
  void expandMedia(Json::Value& input, Json::Value& analyzer);
  void expandAnalyzer(Json::Value& source, int index, Json::Value& analyzer);
  bool reserveOutputDirectory(const std::filesystem::path& path);

  drogon::orm::DbClientPtr dbClient;
};
