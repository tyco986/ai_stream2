#pragma once

#include <drogon/utils/coroutine.h>

#include <string>

enum class PathReplace { mounted, unreachable, rejected };

class MediaMtx {
 public:
  static drogon::Task<PathReplace> replacePath(const std::string& name, const std::string& source, bool record);
  static drogon::Task<bool> deletePath(const std::string& name);
  static drogon::Task<bool> getPath(const std::string& name);

 private:
  // A published file is pushed to this MediaMTX path, so the path must accept a publisher.
  // Callers still pass the RTSP url. Writing that url as source would make MediaMTX pull itself.
  static std::string selfPublishSource(const std::string& name, const std::string& source);
};
