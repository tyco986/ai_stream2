#include "pipeline_service.hpp"

#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>

#include "api_error.hpp"

ChildProcess::ChildProcess() : pid_(-1) {}

void ChildProcess::spawn(const std::string& runner, const std::string& config_dir) {
  const pid_t pid = fork();
  if (pid < 0) {
    throw ApiError("fork failed", 500);
  }
  if (pid == 0) {
    execl(runner.c_str(), "pipeline_runner", config_dir.c_str(),
          static_cast<char*>(nullptr));
    _exit(127);
  }
  pid_ = pid;
}

bool ChildProcess::running() {
  bool alive = false;
  if (pid_ > 0) {
    int status = 0;
    const pid_t waited = waitpid(pid_, &status, WNOHANG);
    if (waited == 0) {
      alive = true;
    } else {
      pid_ = -1;
    }
  }
  return alive;
}

PipelineService::PipelineService(std::string runner_path)
    : runner_path_(std::move(runner_path)),
      child_(),
      pipeline_name_(),
      pipeline_type_() {}

YAML::Node PipelineService::status() {
  YAML::Node data;
  data["pipeline_running"] = child_.running();
  if (pipeline_name_.empty()) {
    data["name"] = YAML::Node();
  } else {
    data["name"] = pipeline_name_;
  }
  if (pipeline_type_.empty()) {
    data["type"] = YAML::Node();
  } else {
    data["type"] = pipeline_type_;
  }
  return data;
}

YAML::Node PipelineService::start(const std::string& raw) {
  const YAML::Node config = YAML::Load(raw);
  if (!config || !config.IsMap()) {
    throw ApiError("pipeline YAML must be a mapping", 400);
  }
  const std::string type = config["type"].as<std::string>();
  const std::string config_dir = config["config_dir"].as<std::string>();
  if (child_.running()) {
    throw ApiError("pipeline is running", 400);
  }
  const std::filesystem::path spec = std::filesystem::path(config_dir) / "pipeline.yml";
  const std::filesystem::path params = std::filesystem::path(config_dir) / "params.yml";
  if (!std::filesystem::is_regular_file(spec)) {
    throw ApiError("missing pipeline.yml in config_dir", 400);
  }
  if (!std::filesystem::is_regular_file(params)) {
    throw ApiError("missing params.yml in config_dir", 400);
  }
  const YAML::Node params_node = YAML::LoadFile(params.string());
  if (!params_node || !params_node["pipeline_name"]) {
    throw ApiError("params.yml missing pipeline_name", 400);
  }
  const std::string name = params_node["pipeline_name"].as<std::string>();
  child_.spawn(runner_path_, config_dir);
  pipeline_name_ = name;
  pipeline_type_ = type;
  YAML::Node data;
  data["name"] = name;
  data["type"] = type;
  return data;
}
