#ifndef PIPELINE_RUNNER_HPP
#define PIPELINE_RUNNER_HPP

#include <string>
#include <vector>

#include "pipeline.hpp"
#include "yaml-cpp/yaml.h"

class PipelineRunner {
 public:
  explicit PipelineRunner(std::string config_dir);
  void run();

 private:
  std::string config_dir_;
  std::string config_path_;
  std::string pipeline_name_;
  YAML::Node spec_;
  deepstream::Pipeline pipeline_;

  static std::string loadPipelineName(const std::string& config_dir);
  void linkRequestPads();
  void linkMuxPads();
  void linkDemuxPads();
  std::vector<std::string> collectNames(const std::string& prefix, bool allow_exact) const;
  bool yamlEdgeExists(const std::string& src, const std::string& dst) const;
};

#endif
