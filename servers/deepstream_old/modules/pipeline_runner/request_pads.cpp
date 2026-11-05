#include "pipeline_runner.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace {

bool suffixIsDigits(const std::string& name, size_t prefix_size) {
  bool digits = !name.empty() && prefix_size < name.size();
  for (size_t i = prefix_size; digits && i < name.size(); ++i) {
    digits = std::isdigit(static_cast<unsigned char>(name[i])) != 0;
  }
  return digits;
}

bool nameMatches(const std::string& name, const std::string& prefix, bool allow_exact) {
  bool matched = allow_exact && name == prefix;
  if (!matched && name.size() > prefix.size() &&
      name.compare(0, prefix.size(), prefix) == 0) {
    matched = suffixIsDigits(name, prefix.size());
  }
  return matched;
}

int nameIndex(const std::string& name, const std::string& prefix) {
  int index = 0;
  if (name.size() > prefix.size()) {
    index = std::stoi(name.substr(prefix.size()));
  }
  return index;
}

struct NumericSuffixLess {
  const std::string* prefix;
  bool operator()(const std::string& left, const std::string& right) const {
    return nameIndex(left, *prefix) < nameIndex(right, *prefix);
  }
};

}  // namespace

std::vector<std::string> PipelineRunner::collectNames(
    const std::string& prefix, bool allow_exact) const {
  std::vector<std::string> names;
  const YAML::Node nodes = spec_["deepstream"]["nodes"];
  for (const YAML::Node& node : nodes) {
    const std::string name = node["name"].as<std::string>();
    if (nameMatches(name, prefix, allow_exact)) {
      names.push_back(name);
    }
  }
  std::sort(names.begin(), names.end(), NumericSuffixLess{&prefix});
  return names;
}

bool PipelineRunner::yamlEdgeExists(const std::string& src, const std::string& dst) const {
  bool exists = false;
  const YAML::Node edges = spec_["deepstream"]["edges"];
  if (edges && edges[src]) {
    exists = edges[src].as<std::string>() == dst;
  }
  return exists;
}

void PipelineRunner::linkMuxPads() {
  if (pipeline_.find("nvstreammux") != nullptr) {
    const std::vector<std::string> sources = collectNames("nvurisrcbin", true);
    for (const std::string& src : sources) {
      if (!yamlEdgeExists(src, "nvstreammux")) {
        pipeline_.link(std::make_pair(src, std::string("nvstreammux")),
                       std::make_pair(std::string(""), std::string("sink_%u")));
      }
    }
  }
}

void PipelineRunner::linkDemuxPads() {
  if (pipeline_.find("nvstreamdemux") != nullptr) {
    const std::vector<std::string> queues = collectNames("queue_demux", false);
    for (const std::string& queue : queues) {
      pipeline_.link(std::make_pair(std::string("nvstreamdemux"), queue),
                     std::make_pair(std::string("src_%u"), std::string("")));
    }
  }
}

void PipelineRunner::linkRequestPads() {
  linkMuxPads();
  linkDemuxPads();
}
