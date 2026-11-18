#include "logger/kafka_logger.hpp"

#include "constants/kafka_const.hpp"
#include "constants/response_const.hpp"
#include "constants/service_log.hpp"
#include "logger/base_logger.hpp"

#include <json/json.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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
  if (BaseLogger::enabledValue()) {
    Json::Value entry(Json::objectValue);
    entry[kafka::KEY_TS] = timestamp();
    entry[kafka::KEY_TOPIC] = topic;
    entry[kafka::KEY_PAYLOAD] = payload;
    Json::StreamWriterBuilder writer;
    writer[http::JSON_INDENTATION] = "";
    writer[http::JSON_PRECISION_TYPE] = http::JSON_PRECISION_DECIMAL;
    writer[http::JSON_PRECISION] = http::JSON_PRECISION_DIGITS;
    std::string line = Json::writeString(writer, entry);
    if (!line.empty() && line.back() == '\n') {
      line.pop_back();
    }
    BaseLogger::write(service_log::KAFKA_DIR, line);
  }
}
