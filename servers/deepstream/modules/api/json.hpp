#ifndef API_JSON_HPP
#define API_JSON_HPP

#include <string>

#include "yaml-cpp/yaml.h"

class YamlJson {
 public:
  static std::string dump(const YAML::Node& node);
  static std::string escape(const std::string& text);
  static std::string envelope(bool success, const std::string& message,
                              const std::string& data_json);

 private:
  static std::string dumpScalar(const YAML::Node& node);
  static std::string dumpSequence(const YAML::Node& node);
  static std::string dumpMap(const YAML::Node& node);
  static bool isJsonNumber(const std::string& text);
};

#endif
