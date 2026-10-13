#pragma once

#include <drogon/utils/coroutine.h>

#include <string>

class MediaMtx {
 public:
  static drogon::Task<bool> replacePath(const std::string& name, const std::string& source, bool record);
  static drogon::Task<bool> deletePath(const std::string& name);
  static drogon::Task<bool> getPath(const std::string& name);
};
