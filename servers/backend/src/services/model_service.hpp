#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>
#include <vector>

struct BuildStart {
  std::string exportOnnx;
  std::string exportTrt;
};

struct ExportResponse {
  Json::Value data;
  std::string fileName;
  std::string fileContent;
};

struct ImportFile {
  std::string fileName;
  std::string fileContent;
};

class ModelService {
 public:
  explicit ModelService(drogon::orm::DbClientPtr dbClient);
  Json::Value list(const std::string& search, int page, long pageSize);
  Json::Value get(const std::string& modelId);
  Json::Value remove(const std::string& modelId);
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
  ExportResponse buildParse(const std::string& contentType, const std::string& body);
  Json::Value buildEnd(const std::string& modelId,
                       const Json::Value& data,
                const std::string& fileName = "",
                const std::string& fileContent = "");
  Json::Value upload(const std::string& modelId, const std::string& fileName, const std::string& fileContent);
  Json::Value importModel(const std::vector<ImportFile>& files);
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
  bool createCheck(const std::string& name,
                const std::string& type,
                int batchSize,
                const std::string& batchMode,
                const std::string& precision);
  Json::Value row2json(const drogon::orm::Row& row);
  std::string json2yaml(const Json::Value& fields);
  bool uploadCheck(const std::string& path);
  bool importCheck(const std::string& bytes, Json::Value& meta);

  drogon::orm::DbClientPtr dbClient;
};
