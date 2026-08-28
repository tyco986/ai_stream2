#include "api_server.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <utility>

#include "api_error.hpp"
#include "json.hpp"
#include "yaml-cpp/yaml.h"

FileLogger::FileLogger(std::string path)
    : path_(std::move(path)), file_(nullptr), mutex_() {
  std::filesystem::create_directories(std::filesystem::path(path_).parent_path());
  file_ = std::fopen(path_.c_str(), "a");
}

FileLogger::~FileLogger() {
  if (file_ != nullptr) {
    std::fclose(file_);
  }
}

void FileLogger::info(const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;
  std::tm tm{};
  localtime_r(&t, &tm);
  std::ostringstream stamp;
  stamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "," << std::setw(3)
        << std::setfill('0') << ms.count();
  if (file_ != nullptr) {
    std::fprintf(file_, "[%s] INFO deepstream_api: %s\n", stamp.str().c_str(),
                 message.c_str());
    std::fflush(file_);
  }
}

ApiServer::ApiServer(AppConfig config, PipelineService& service)
    : config_(std::move(config)),
      service_(service),
      logger_(config_.log_root + "/app.log"),
      server_(),
      prefix_("/" + config_.project_name + "/deepstream") {
  bindRoutes();
}

void ApiServer::bindRoutes() {
  server_.Get(prefix_ + "/health",
              [this](const httplib::Request& req, httplib::Response& res) {
                handleHealth(req, res);
              });
  server_.Get(prefix_ + "/pipeline/status",
              [this](const httplib::Request& req, httplib::Response& res) {
                handleStatus(req, res);
              });
  server_.Get(prefix_ + "/types",
              [this](const httplib::Request& req, httplib::Response& res) {
                handleTypes(req, res);
              });
  server_.Post(prefix_ + "/schema",
               [this](const httplib::Request& req, httplib::Response& res) {
                 handleSchema(req, res);
               });
  server_.Post(prefix_ + "/start_pipeline",
               [this](const httplib::Request& req, httplib::Response& res) {
                 handleStart(req, res);
               });
  server_.set_logger([this](const httplib::Request& req, const httplib::Response& res) {
    logRequest(req, res);
  });
}

void ApiServer::writeResult(httplib::Response& res, int status, bool success,
                            const std::string& message, const std::string& data_json) {
  res.status = status;
  res.set_content(YamlJson::envelope(success, message, data_json), "application/json");
}

void ApiServer::finish(httplib::Response& res, const std::function<std::string()>& body) {
  int status = 200;
  bool success = true;
  std::string message;
  std::string data = "null";
  try {
    data = body();
  } catch (const ApiError& err) {
    status = err.status_code();
    success = false;
    message = err.what();
  } catch (const YAML::Exception& err) {
    status = 400;
    success = false;
    message = err.what();
  } catch (const std::exception&) {
    status = 500;
    success = false;
    message = "internal error";
  }
  writeResult(res, status, success, message, data);
}

void ApiServer::handleHealth(const httplib::Request&, httplib::Response& res) {
  finish(res, [] {
    return std::string("null");
  });
}

void ApiServer::handleStatus(const httplib::Request&, httplib::Response& res) {
  finish(res, [this] {
    return YamlJson::dump(service_.status());
  });
}

void ApiServer::handleTypes(const httplib::Request&, httplib::Response& res) {
  finish(res, [this] {
    return YamlJson::dump(service_.types());
  });
}

void ApiServer::handleSchema(const httplib::Request& req, httplib::Response& res) {
  finish(res, [this, &req] {
    const YAML::Node body = YAML::Load(req.body);
    std::string pipeline_type;
    if (body && body["pipeline_type"]) {
      pipeline_type = body["pipeline_type"].as<std::string>();
    } else {
      throw ApiError("pipeline_type is required", 400);
    }
    return YamlJson::dump(service_.schema(pipeline_type));
  });
}

void ApiServer::handleStart(const httplib::Request& req, httplib::Response& res) {
  finish(res, [this, &req] {
    if (!req.has_file("input")) {
      throw ApiError("missing file field input", 400);
    }
    const httplib::MultipartFormData file = req.get_file_value("input");
    return YamlJson::dump(service_.start(file.filename, file.content));
  });
}

void ApiServer::logRequest(const httplib::Request& req, const httplib::Response& res) {
  logger_.info("request " + req.method + " " + req.path + " -> " +
               std::to_string(res.status));
}

bool ApiServer::listen() {
  const bool ok = server_.listen(config_.host.c_str(), config_.port);
  return ok;
}
