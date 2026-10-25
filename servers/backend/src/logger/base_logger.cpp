#include "logger/base_logger.hpp"

#include "constants/service_log.hpp"
#include "constants/response_const.hpp"

#include <json/json.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>

#include <cctype>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <sstream>
#include <vector>

Json::StreamWriterBuilder BaseLogger::makeWriter() {
  Json::StreamWriterBuilder builder;
  builder[http::JSON_EMIT_UTF8] = true;
  builder[http::JSON_INDENTATION] = "";
  return builder;
}

Json::StreamWriterBuilder BaseLogger::writer = BaseLogger::makeWriter();
bool BaseLogger::enabled = false;
std::mutex BaseLogger::mutex;
std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> BaseLogger::loggers;

void BaseLogger::setEnabled(bool value) {
  std::lock_guard<std::mutex> guard(mutex);
  enabled = value;
}

bool BaseLogger::enabledValue() {
  std::lock_guard<std::mutex> guard(mutex);
  return enabled;
}

Json::Value BaseLogger::readLines(std::istream& input) {
  Json::Value lines(Json::arrayValue);
  for (std::string line; std::getline(input, line); ) {
    Json::Value item;
    const bool parsed = !line.empty() && Json::Reader().parse(line, item);
    const bool latencyField = parsed && item[service_log::KEY_LATENCY].isNumeric();
    std::ostringstream latency;
    latency << std::fixed << std::setprecision(service_log::LATENCY_DIGITS)
            << (latencyField ? item[service_log::KEY_LATENCY].asDouble() : 0.0);
    if (latencyField) {
      item[service_log::KEY_LATENCY] = latency.str();
    }
    if (parsed) {
      lines.append(item);
    }
  }
  return lines;
}

Json::Value BaseLogger::get(const std::string& page, const std::string& date) {
  bool dateOk = date.empty() || date.size() == service_log::DATE_LENGTH;
  for (std::size_t index = 0; dateOk && !date.empty() && index < date.size(); ++index) {
    const bool separator = index == 4 || index == 7;
    const unsigned char character = static_cast<unsigned char>(date[index]);
    dateOk = separator ? date[index] == '-' : std::isdigit(character) != 0;
  }
  bool pageOk = page.empty();
  for (const char* item : service_log::PAGES) {
    pageOk = pageOk || page == item;
  }
  std::vector<const char*> targets;
  if (pageOk && dateOk && page.empty()) {
    targets.assign(std::begin(service_log::PAGES), std::end(service_log::PAGES));
  }
  if (pageOk && dateOk && !page.empty()) {
    targets.push_back(page.c_str());
  }
  Json::Value result(Json::objectValue);
  for (const char* item : targets) {
    Json::Value dates(Json::objectValue);
    const std::filesystem::path directory = std::filesystem::path(service_log::DIR) / item;
    if (!date.empty()) {
      const std::filesystem::path direct =
          directory / (std::string(item) + "_" + date + service_log::FILE_SUFFIX);
      std::ifstream input(direct);
      const bool directHit = static_cast<bool>(input);
      if (directHit) {
        Json::Value body(Json::objectValue);
        body[service_log::KEY_CONTENT] = readLines(input);
        dates[date] = body;
      }
    }
    const bool scan = date.empty() && std::filesystem::is_directory(directory);
    if (scan) {
      const std::string prefix = std::string(item) + "_";
      const std::string suffix = service_log::FILE_SUFFIX;
      for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory)) {
        const std::string filename = entry.path().filename().string();
        const bool shaped = entry.is_regular_file() &&
                            filename.size() == prefix.size() + service_log::DATE_LENGTH + suffix.size() &&
                            filename.compare(0, prefix.size(), prefix) == 0 &&
                            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
        const std::string stamp = shaped ? filename.substr(prefix.size(), service_log::DATE_LENGTH) : "";
        bool stampOk = shaped;
        for (std::size_t index = 0; stampOk && index < stamp.size(); ++index) {
          const bool separator = index == 4 || index == 7;
          const unsigned char character = static_cast<unsigned char>(stamp[index]);
          stampOk = separator ? stamp[index] == '-' : std::isdigit(character) != 0;
        }
        if (stampOk) {
          std::ifstream file(entry.path());
          Json::Value body(Json::objectValue);
          body[service_log::KEY_CONTENT] = readLines(file);
          dates[stamp] = body;
        }
      }
    }
    result[item] = dates;
  }
  return result;
}

Json::Value BaseLogger::remove(const std::string& page) {
  std::lock_guard<std::mutex> guard(mutex);
  bool pageOk = page.empty();
  for (const char* item : service_log::PAGES) {
    pageOk = pageOk || page == item;
  }
  std::vector<const char*> targets;
  if (pageOk && page.empty()) {
    targets.assign(std::begin(service_log::PAGES), std::end(service_log::PAGES));
  }
  if (pageOk && !page.empty()) {
    targets.push_back(page.c_str());
  }
  Json::Value result(Json::objectValue);
  for (const char* item : targets) {
    const auto found = loggers.find(item);
    const bool opened = found != loggers.end();
    if (opened) {
      found->second->flush();
    }
    loggers.erase(item);
    const std::filesystem::path directory = std::filesystem::path(service_log::DIR) / item;
    const bool exists = std::filesystem::is_directory(directory);
    const std::uintmax_t removed = exists ? std::filesystem::remove_all(directory) : 0;
    const std::uintmax_t count = removed > 0 ? removed - 1 : 0;
    result[item] = static_cast<Json::UInt64>(count);
  }
  return result;
}

std::string BaseLogger::formatLog(const Json::Value& body) {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::tm local{};
  localtime_r(&time, &local);
  std::ostringstream timestamp;
  timestamp << std::put_time(&local, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3)
            << milliseconds.count();

  Json::Value entry = body;
  entry[service_log::KEY_TS] = timestamp.str();
  std::string text = "{";
  for (std::size_t index = 0; index < service_log::LINE_KEY_COUNT; ++index) {
    const bool later = index > 0;
    if (later) {
      text += ",";
    }
    const char* key = service_log::LINE_KEYS[index];
    Json::Value parsed;
    const bool bodyField = key == service_log::KEY_INPUT || key == service_log::KEY_OUTPUT;
    const std::string raw = bodyField && entry[key].isString() ? entry[key].asString() : "";
    const bool parsedOk = bodyField && !raw.empty() && Json::Reader().parse(raw, parsed);
    const Json::Value field = parsedOk ? parsed : entry[key];
    std::string encoded = Json::writeString(writer, field);
    const bool trailingNewline = !encoded.empty() && encoded.back() == '\n';
    if (trailingNewline) {
      encoded.pop_back();
    }
    const bool latencyField = key == service_log::KEY_LATENCY && entry[key].isNumeric();
    std::ostringstream latency;
    latency << std::fixed << std::setprecision(service_log::LATENCY_DIGITS)
            << (latencyField ? entry[key].asDouble() : 0.0);
    encoded = latencyField ? latency.str() : encoded;
    text += "\"";
    text += key;
    text += "\":";
    text += encoded;
  }
  text += "}";
  return text;
}

void BaseLogger::write(const std::string& text) {
  std::lock_guard<std::mutex> guard(mutex);
  if (enabled) {
    const std::string loggerName = name();
    const bool ready = loggers.find(loggerName) != loggers.end();
    if (!ready) {
      const std::string directoryPath = std::string(service_log::DIR) + "/" + name();
      std::filesystem::create_directories(directoryPath);
      const std::string path = directoryPath + "/" + name() + service_log::FILE_SUFFIX;
      auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
          path, service_log::ROTATION_HOUR, service_log::ROTATION_MINUTE, false, service_log::MAX_DAYS);
      auto created = std::make_shared<spdlog::logger>(loggerName, sink);
      created->set_pattern("%v");
      created->flush_on(spdlog::level::info);
      loggers[loggerName] = created;
    }
    loggers[loggerName]->info("{}", text);
  }
}

void BaseLogger::respond(const drogon::HttpRequestPtr& request,
                         const drogon::HttpResponsePtr& response,
                         std::chrono::steady_clock::time_point started,
                         const std::function<void(const drogon::HttpResponsePtr&)>& callback,
                         const std::string& error) {
  const auto elapsed = std::chrono::steady_clock::now() - started;
  const std::string query = request->query();
  Json::Value body(Json::objectValue);
  body[service_log::KEY_METHOD] = request->methodString();
  body[service_log::KEY_URL] = request->path() + (query.empty() ? "" : "?" + query);
  body[service_log::KEY_CODE] = static_cast<int>(response->statusCode());
  body[service_log::KEY_INPUT] = std::string(request->getBody());
  body[service_log::KEY_OUTPUT] = std::string(response->getBody());
  body[service_log::KEY_LATENCY] = std::chrono::duration<double, std::milli>(elapsed).count();
  body[service_log::KEY_IP] = request->peerAddr().toIp();
  body[service_log::KEY_IN_BYTES] = static_cast<Json::UInt64>(request->bodyLength());
  body[service_log::KEY_OUT_BYTES] = static_cast<Json::UInt64>(response->getBody().size());
  body[service_log::KEY_ERROR] = error;
  body[service_log::KEY_HEADER] = Json::Value(Json::objectValue);
  for (std::size_t index = 0; index < service_log::HEADER_COUNT; ++index) {
    const char* name = service_log::HEADERS[index];
    const std::string value = request->getHeader(name);
    const bool present = !value.empty();
    if (present) {
      body[service_log::KEY_HEADER][name] = value;
    }
  }
  write(formatLog(body));
  callback(response);
}
