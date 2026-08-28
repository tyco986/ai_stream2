#ifndef API_SERVER_HPP
#define API_SERVER_HPP

#include <cstdio>
#include <functional>
#include <mutex>
#include <string>

#include "httplib.h"

#include "constants.hpp"
#include "pipeline_service.hpp"

class FileLogger {
 public:
  explicit FileLogger(std::string path);
  ~FileLogger();
  FileLogger(const FileLogger&) = delete;
  FileLogger& operator=(const FileLogger&) = delete;

  void info(const std::string& message);

 private:
  std::string path_;
  FILE* file_;
  std::mutex mutex_;
};

class ApiServer {
 public:
  ApiServer(AppConfig config, PipelineService& service);
  ~ApiServer() = default;

  bool listen();

 private:
  AppConfig config_;
  PipelineService& service_;
  FileLogger logger_;
  httplib::Server server_;
  std::string prefix_;

  void bindRoutes();
  void handleHealth(const httplib::Request& req, httplib::Response& res);
  void handleStatus(const httplib::Request& req, httplib::Response& res);
  void handleTypes(const httplib::Request& req, httplib::Response& res);
  void handleSchema(const httplib::Request& req, httplib::Response& res);
  void handleStart(const httplib::Request& req, httplib::Response& res);
  void finish(httplib::Response& res, const std::function<std::string()>& body);
  void writeResult(httplib::Response& res, int status, bool success,
                   const std::string& message, const std::string& data_json);
  void logRequest(const httplib::Request& req, const httplib::Response& res);
};

#endif
