#include "pipeline_service.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

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

PipelineService::PipelineService(std::string schema_dir, std::string config_save_dir,
                                 std::string runner_path)
    : schema_dir_(std::move(schema_dir)),
      config_save_dir_(std::move(config_save_dir)),
      runner_path_(std::move(runner_path)),
      schemas_(),
      child_(),
      pipeline_name_(),
      pipeline_type_() {
  loadSchemas();
}

void PipelineService::loadSchemas() {
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(schema_dir_)) {
    const std::string ext = entry.path().extension().string();
    if (entry.is_regular_file() && (ext == ".yaml" || ext == ".yml")) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  for (const std::filesystem::path& path : files) {
    const YAML::Node data = YAML::LoadFile(path.string());
    if (!data || !data.IsMap()) {
      throw std::runtime_error("schema YAML must be a mapping: " + path.string());
    }
    if (!data["type"]) {
      throw std::runtime_error("schema YAML missing type: " + path.string());
    }
    const std::string pipeline_type = data["type"].as<std::string>();
    if (schemas_.find(pipeline_type) != schemas_.end()) {
      throw std::runtime_error("duplicate schema type: " + pipeline_type);
    }
    schemas_[pipeline_type] = data;
  }
}

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

YAML::Node PipelineService::types() const {
  YAML::Node data;
  YAML::Node items(YAML::NodeType::Sequence);
  for (const auto& entry : schemas_) {
    items.push_back(entry.first);
  }
  data["items"] = items;
  return data;
}

YAML::Node PipelineService::schema(const std::string& pipeline_type) const {
  YAML::Node result;
  const auto it = schemas_.find(pipeline_type);
  if (it == schemas_.end()) {
    throw ApiError("unknown type '" + pipeline_type + "'", 404);
  }
  result = it->second;
  return result;
}

void PipelineService::saveConfig(const std::string& filename,
                                 const std::string& raw) const {
  std::filesystem::path name = std::filesystem::path(filename).filename();
  if (name.empty() || name == "." || name == "..") {
    name = "pipeline.yaml";
  }
  std::filesystem::create_directories(config_save_dir_);
  const std::filesystem::path path = std::filesystem::path(config_save_dir_) / name;
  std::ofstream out(path, std::ios::binary);
  out.write(raw.data(), static_cast<std::streamsize>(raw.size()));
}

YAML::Node PipelineService::start(const std::string& filename, const std::string& raw) {
  saveConfig(filename, raw);
  const YAML::Node config = YAML::Load(raw);
  if (!config || !config.IsMap()) {
    throw ApiError("pipeline YAML must be a mapping", 400);
  }
  const std::string type = config["type"].as<std::string>();
  const std::string config_dir = config["config_dir"].as<std::string>();
  if (schemas_.find(type) == schemas_.end()) {
    throw ApiError("unknown type '" + type + "'", 400);
  }
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
