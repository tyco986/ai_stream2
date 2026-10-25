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
};
