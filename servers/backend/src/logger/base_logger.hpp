#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <spdlog/logger.h>

#include <chrono>
#include <functional>
#include <istream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class BaseLogger {
 public:
  virtual ~BaseLogger() = default;
  static void setEnabled(bool value);
  static bool enabledValue();
  static Json::Value get(const std::string& page, const std::string& date);
  static Json::Value remove(const std::string& page);
  void respond(const drogon::HttpRequestPtr& request,
               const drogon::HttpResponsePtr& response,
               std::chrono::steady_clock::time_point started,
               const std::function<void(const drogon::HttpResponsePtr&)>& callback,
               const std::string& error = "");

 protected:
  virtual const char* name() const = 0;

 private:
  static Json::StreamWriterBuilder makeWriter();
  static Json::Value readLines(std::istream& input);
  static std::string formatLog(const Json::Value& body);
  void write(const std::string& text);

  static Json::StreamWriterBuilder writer;
  static bool enabled;
  static std::mutex mutex;
  static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;
};
