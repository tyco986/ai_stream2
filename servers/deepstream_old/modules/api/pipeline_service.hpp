#ifndef PIPELINE_SERVICE_HPP
#define PIPELINE_SERVICE_HPP

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
  explicit PipelineService(std::string runner_path);

  YAML::Node status();
  YAML::Node start(const std::string& config_dir);

 private:
  std::string runner_path_;
  ChildProcess child_;
  std::string pipeline_name_;
  std::string pipeline_type_;
};

#endif
