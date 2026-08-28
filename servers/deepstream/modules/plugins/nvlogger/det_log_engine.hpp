#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>

#include "nvdsmeta.h"

namespace nvlogger {

class DetLogEngine {
 public:
  DetLogEngine();
  ~DetLogEngine();

  DetLogEngine(const DetLogEngine &) = delete;
  DetLogEngine &operator=(const DetLogEngine &) = delete;

  void set_root(const char *root);
  void set_interval(int interval);
  const char *root() const;
  int interval() const;

  bool process_frame(NvDsFrameMeta *frame_meta, double latency_ms);

 private:
  bool should_log(int pad);
  FILE *file_for_pad(int pad, bool *ok);
  bool write_line(int pad, const std::string &line);
  std::string build_line(NvDsFrameMeta *frame_meta, double latency_ms) const;
  std::string escape_label(const char *label) const;

  std::string root_;
  int interval_ = 0;
  std::unordered_map<int, int> counters_;
  std::unordered_map<int, FILE *> files_;
};

}  // namespace nvlogger
