#pragma once

#include <drogon/orm/DbClient.h>
#include <json/json.h>

#include <string>

class EventService {
 public:
  explicit EventService(drogon::orm::DbClientPtr dbClient);
  Json::Value get(const std::string& eventId);
  Json::Value createFromRedis(const Json::Value& message, const std::string& topic);
  Json::Value create(const Json::Value& occurredAt,
                     const Json::Value& streamId,
                     const Json::Value& mediaId,
                     const Json::Value& pipelineId,
                     const Json::Value& eventCode,
                     const Json::Value& level,
                     const Json::Value& eventLabel,
                     const Json::Value& rawPath,
                     const Json::Value& visualizationPath,
                     const Json::Value& annotations);
  Json::Value setRead(const std::string& eventId, bool read);
  Json::Value remove(const std::string& eventId);
  Json::Value list(const std::string& search, const std::string& level, int page, long pageSize);
  std::string contentPath(const std::string& eventId, const std::string& kind);
  Json::Value contentInfo(const std::string& eventId, const std::string& kind);

 private:
  Json::Value row2json(const drogon::orm::Row& row);

  drogon::orm::DbClientPtr dbClient;
};
