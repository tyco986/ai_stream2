#pragma once

#include <spdlog/logger.h>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

struct LogEntry {
  std::string method;
  std::string url;
  int code;
  std::string input;
  std::string output;
  double latency;
  std::string ip;
  std::size_t inBytes;
  std::size_t outBytes;
};

class BaseLogger {
 public:
  virtual ~BaseLogger() = default;
  static void setEnabled(bool value);
  void write(const LogEntry& entry);

 protected:
  virtual const char* directory() const = 0;
  virtual const char* name() const = 0;

 private:
  void open();
  std::string line(const LogEntry& entry);

  static bool enabled;
  static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;
};
