#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class EventService {
 public:
  explicit EventService(drogon::orm::DbClientPtr dbClient);
  Json::Value get(const std::string& eventId);
  Json::Value create(const Json::Value& occurredAt,
                     const Json::Value& streamId,
                     const Json::Value& streamName,
                     const Json::Value& pipelineId,
                     const Json::Value& pipelineName,
                     const Json::Value& eventCode,
                     const Json::Value& eventLabel,
                     const Json::Value& status,
                     const Json::Value& rawPath,
                     const Json::Value& visualizationPath,
                     const Json::Value& annotations);
  Json::Value setStatus(const std::string& eventId, const std::string& status);
  Json::Value list(const std::string& search, int page, int pageSize);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
