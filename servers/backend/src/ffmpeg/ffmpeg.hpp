#pragma once

#include <drogon/utils/coroutine.h>
#include <json/json.h>

#include <string>

struct ProbeResult {
  bool accepted;
  Json::Value data;
};

class Ffmpeg {
 public:
  static drogon::Task<ProbeResult> rtspProbe(const std::string& url);
  static drogon::Task<ProbeResult> rtspPublished();
  static drogon::Task<ProbeResult> rtspPublish(const std::string& name, const std::string& path);
  static drogon::Task<ProbeResult> rtspUnpublish(const std::string& name);
};
