#pragma once

#include <drogon/HttpRequest.h>

#include <string>
#include <unordered_map>

class I18n {
 public:
  static void load(const std::string& directory);
  static void bind(const drogon::HttpRequestPtr& request);
  static const char* text(const char* key);

 private:
  static void language(const std::string& header);

  static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> catalog;
  static thread_local std::string current;
};
