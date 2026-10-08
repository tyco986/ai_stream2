#include "log/base_logger.hpp"

#include "constants/log.hpp"

#include <json/json.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

bool BaseLogger::enabled = false;
std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> BaseLogger::loggers;

void BaseLogger::setEnabled(bool value) {
  enabled = value;
}

void BaseLogger::open() {
  const std::string loggerName = name();
  const bool ready = loggers.find(loggerName) != loggers.end();
  if (!ready) {
    const std::string directoryPath = std::string(log::DIR) + "/" + directory();
    std::filesystem::create_directories(directoryPath);
    const std::string path = directoryPath + "/" + directory() + log::FILE_SUFFIX;
    auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        path, log::ROTATION_HOUR, log::ROTATION_MINUTE, false, log::MAX_DAYS);
    auto created = std::make_shared<spdlog::logger>(loggerName, sink);
    created->set_pattern("%v");
    loggers[loggerName] = created;
  }
}

std::string BaseLogger::line(const LogEntry& entry) {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::tm local{};
  localtime_r(&time, &local);
  std::ostringstream timestamp;
  timestamp << std::put_time(&local, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3)
            << milliseconds.count();

  Json::Value body;
  body[log::KEY_TS] = timestamp.str();
  body[log::KEY_METHOD] = entry.method;
  body[log::KEY_URL] = entry.url;
  body[log::KEY_CODE] = entry.code;
  body[log::KEY_INPUT] = entry.input;
  body[log::KEY_OUTPUT] = entry.output;
  body[log::KEY_LATENCY] = entry.latency;
  body[log::KEY_IP] = entry.ip;
  body[log::KEY_IN_BYTES] = static_cast<Json::UInt64>(entry.inBytes);
  body[log::KEY_OUT_BYTES] = static_cast<Json::UInt64>(entry.outBytes);

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  std::string text = Json::writeString(builder, body);
  const bool trailingNewline = !text.empty() && text.back() == '\n';
  if (trailingNewline) {
    text.pop_back();
  }
  return text;
}

void BaseLogger::write(const LogEntry& entry) {
  if (enabled) {
    open();
    loggers[name()]->info(line(entry));
  }
}
