#ifndef API_CONSTANTS_HPP
#define API_CONSTANTS_HPP

#include <string>

inline constexpr const char* kConfigDirKey = "config_dir";
inline constexpr const char* kParamsFile = "params.yml";
inline constexpr const char* kPipelineFile = "pipeline.yml";

class AppConfig {
 public:
  AppConfig();

  std::string project_name;
  std::string host;
  int port;
  std::string log_root;
  std::string pipeline_runner;

 private:
  static std::string envValue(const char* key, const char* fallback);
  static int envInt(const char* key, int fallback);
};

#endif
