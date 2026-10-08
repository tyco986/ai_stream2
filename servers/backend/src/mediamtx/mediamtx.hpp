#pragma once

#include <functional>
#include <string>

class MediaMtx {
 public:
  static void replacePath(const std::string& name,
                          const std::string& source,
                          bool record,
                          std::function<void(bool)> callback);
  static void deletePath(const std::string& name, std::function<void(bool)> callback);
};
