#include "constants.hpp"

#include <cstdlib>
#include <string>

AppConfig::AppConfig()
    : project_name(envValue("PROJECT_NAME", "ai_stream2")),
      host(envValue("HOST", "0.0.0.0")),
      port(envInt("PORT", 8092)),
      log_root(envValue("LOG_ROOT", "/root/logs/deepstream")),
      config_save_dir(envValue("CONFIG_SAVE_DIR", "/root/configs/deepstream")),
      schema_dir(envValue("SCHEMA_DIR", "/app/schemas")),
      pipeline_runner(envValue("PIPELINE_RUNNER", "/usr/local/bin/pipeline_runner")) {}

std::string AppConfig::envValue(const char* key, const char* fallback) {
  const char* value = std::getenv(key);
  std::string result = fallback;
  if (value != nullptr && value[0] != '\0') {
    result = value;
  }
  return result;
}

int AppConfig::envInt(const char* key, int fallback) {
  const char* value = std::getenv(key);
  int result = fallback;
  if (value != nullptr && value[0] != '\0') {
    result = std::stoi(value);
  }
  return result;
}
