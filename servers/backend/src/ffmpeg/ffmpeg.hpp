#pragma once

#include <functional>
#include <string>

#include <json/json.h>

class Ffmpeg {
 public:
  static void probe(const std::string& url,
                   std::function<void(bool accepted, const Json::Value& data)> callback);
};
