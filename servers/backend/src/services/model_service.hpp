#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

struct BuildStart {
  std::string exportOnnx;
  std::string exportTrt;
};

class ModelService {
 public:
  explicit ModelService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& search, int page, int pageSize);
  Json::Value get(const std::string& modelId);
  void remove(const std::string& modelId);
  Json::Value update(const std::string& modelId,
                     const Json::Value& name,
                     const Json::Value& type,
                     const Json::Value& batchSize,
                     const Json::Value& batchMode,
                     const Json::Value& precision,
                     const Json::Value& optLevel,
                     const Json::Value& device,
                     const Json::Value& maxDet,
                     const Json::Value& conf,
                     const Json::Value& iou);
  Json::Value map();
  BuildStart buildStart(const std::string& modelId);
  void buildEnd(const std::string& modelId, const Json::Value& result);
  Json::Value upload(const std::string& modelId, const std::string& fileName, const std::string& fileContent);
  void importModel(const std::string& fileName, const std::string& fileContent);
  Json::Value create(const Json::Value& name,
                     const Json::Value& type,
                     const Json::Value& batchSize,
                     const Json::Value& batchMode,
                     const Json::Value& precision,
                     const Json::Value& optLevel,
                     const Json::Value& device,
                     const Json::Value& maxDet,
                     const Json::Value& conf,
                     const Json::Value& iou);

 private:
  bool accepted(const std::string& name,
                const std::string& type,
                int batchSize,
                const std::string& batchMode,
                const std::string& precision);
  Json::Value item(const drogon::orm::Row& row);
  std::string yaml(const Json::Value& fields);
  bool checkOnnxZip(const std::string& path);

  drogon::orm::DbClientPtr dbClient;
};
