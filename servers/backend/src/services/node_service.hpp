#pragma once

#include <json/json.h>

class NodeService {
 public:
  Json::Value formatNodesInfo(const Json::Value& systems,
                              const Json::Value& details,
                              const Json::Value& stats);

 private:
  Json::Value byte2MB(const Json::Value& bytes);
  Json::Value formatUptime(const Json::Value& info);
  Json::Value formatGpu(const Json::Value& stats);
  Json::Value textOrNull(const Json::Value& object, const char* key);
  Json::Value intOrNull(const Json::Value& object, const char* key);
};
