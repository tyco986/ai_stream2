#ifndef PIPELINE_SERVICE_HPP
#define PIPELINE_SERVICE_HPP

#include <map>
#include <string>
#include <sys/types.h>

#include "yaml-cpp/yaml.h"

class ChildProcess {
 public:
  ChildProcess();
  ChildProcess(const ChildProcess&) = delete;
  ChildProcess& operator=(const ChildProcess&) = delete;

  void spawn(const std::string& runner, const std::string& config_dir);
  bool running();

 private:
  pid_t pid_;
};

class PipelineService {
 public:
  PipelineService(std::string schema_dir, std::string config_save_dir,
                  std::string runner_path);

  YAML::Node status();
  YAML::Node types() const;
  YAML::Node schema(const std::string& pipeline_type) const;
  YAML::Node start(const std::string& filename, const std::string& raw);

 private:
  std::string schema_dir_;
  std::string config_save_dir_;
  std::string runner_path_;
  std::map<std::string, YAML::Node> schemas_;
  ChildProcess child_;
  std::string pipeline_name_;
  std::string pipeline_type_;

  void loadSchemas();
  void saveConfig(const std::string& filename, const std::string& raw) const;
};

#endif
