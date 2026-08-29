#pragma once

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_map>

#include "nvdsmeta.h"

namespace nvlogger {

class DetLogEngine {
 public:
  DetLogEngine();
  virtual ~DetLogEngine();

  DetLogEngine(const DetLogEngine &) = delete;
  DetLogEngine &operator=(const DetLogEngine &) = delete;

  void set_root(const char *root);
  void set_interval(int interval);
  const char *root() const;
  int interval() const;

  bool process_frame(NvDsFrameMeta *frame_meta, double latency_ms);

 protected:
  virtual const char *log_header() const;
  virtual std::string build_line(NvDsFrameMeta *frame_meta, double latency_ms) const;
  virtual void append_object_item(std::ostringstream &json, NvDsObjectMeta *object_meta) const;
  virtual void append_object_item_tail(std::ostringstream &json, NvDsObjectMeta *object_meta) const;
  std::string escape_label(const char *label) const;

 private:
  bool should_log(int pad);
  FILE *file_for_pad(int pad, bool *ok);
  bool write_line(int pad, const std::string &line);

  std::string root_;
  int interval_ = 0;
  std::unordered_map<int, int> counters_;
  std::unordered_map<int, FILE *> files_;
};

}  // namespace nvlogger
