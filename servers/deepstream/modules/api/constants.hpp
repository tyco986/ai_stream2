#ifndef API_CONSTANTS_HPP
#define API_CONSTANTS_HPP

#include <string>

class AppConfig {
 public:
  AppConfig();

  std::string project_name;
  std::string host;
  int port;
  std::string log_root;
  std::string config_save_dir;
  std::string schema_dir;
  std::string pipeline_runner;

 private:
  static std::string envValue(const char* key, const char* fallback);
  static int envInt(const char* key, int fallback);
};

#endif
