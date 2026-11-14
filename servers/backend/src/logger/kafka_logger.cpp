#include "logger/kafka_logger.hpp"

#include "constants/kafka_const.hpp"
#include "constants/service_log.hpp"
#include "logger/base_logger.hpp"

#include <json/json.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

std::mutex KafkaLogger::logMutex;
std::shared_ptr<spdlog::logger> KafkaLogger::logger;

std::string KafkaLogger::timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::tm local{};
  localtime_r(&time, &local);
  std::ostringstream text;
  text << std::put_time(&local, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3)
       << milliseconds.count();
  return text.str();
}

void KafkaLogger::write(const std::string& topic, const Json::Value& payload) {
  std::lock_guard<std::mutex> guard(logMutex);
  if (BaseLogger::enabledValue()) {
    const bool ready = static_cast<bool>(logger);
    if (!ready) {
      const std::string directoryPath = std::string(service_log::DIR) + "/" + kafka::LOG_DIR;
      std::filesystem::create_directories(directoryPath);
      const std::string path = directoryPath + "/" + kafka::LOG_NAME + service_log::FILE_SUFFIX;
      auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
          path, service_log::ROTATION_HOUR, service_log::ROTATION_MINUTE, false, service_log::MAX_DAYS);
      logger = std::make_shared<spdlog::logger>(kafka::LOG_NAME, sink);
      logger->set_pattern("%v");
      logger->flush_on(spdlog::level::info);
    }
    Json::Value entry(Json::objectValue);
    entry[kafka::KEY_TS] = timestamp();
    entry[kafka::KEY_TOPIC] = topic;
    entry[kafka::KEY_PAYLOAD] = payload;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string line = Json::writeString(writer, entry);
    if (!line.empty() && line.back() == '\n') {
      line.pop_back();
    }
    logger->info("{}", line);
  }
}
