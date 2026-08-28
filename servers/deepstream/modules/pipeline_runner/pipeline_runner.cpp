#include "pipeline_runner.hpp"

#include <filesystem>

std::string PipelineRunner::loadPipelineName(const std::string& config_dir) {
  const YAML::Node params =
      YAML::LoadFile((std::filesystem::path(config_dir) / "params.yml").string());
  return params["pipeline_name"].as<std::string>();
}

PipelineRunner::PipelineRunner(std::string config_dir)
    : config_dir_(std::move(config_dir)),
      config_path_((std::filesystem::path(config_dir_) / "pipeline.yml").string()),
      pipeline_name_(loadPipelineName(config_dir_)),
      spec_(YAML::LoadFile(config_path_)),
      pipeline_(pipeline_name_.c_str(), config_path_) {
  linkRequestPads();
}

void PipelineRunner::run() {
  pipeline_.start().wait();
}
