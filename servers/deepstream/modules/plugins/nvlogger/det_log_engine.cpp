#include "det_log_engine.hpp"

#include "gstnvlogger_common.h"

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>

namespace nvlogger {

namespace {

constexpr uint64_t kUntrackedObjectId = ~static_cast<uint64_t>(0);

int round_coord(float value) {
  return static_cast<int>(std::lround(value));
}

}  // namespace

DetLogEngine::DetLogEngine() {
  root_ = NVLOGGER_PLACEHOLDER_ROOT;
  interval_ = 0;
}

DetLogEngine::~DetLogEngine() {
  for (auto &item : files_) {
    if (item.second != nullptr) {
      fclose(item.second);
    }
  }
}

void DetLogEngine::set_root(const char *root) {
  for (auto &item : files_) {
    if (item.second != nullptr) {
      fclose(item.second);
    }
  }
  files_.clear();
  counters_.clear();
  root_ = (root != nullptr && root[0] != '\0') ? root : NVLOGGER_PLACEHOLDER_ROOT;
}

void DetLogEngine::set_interval(int interval) {
  interval_ = interval < 0 ? 0 : interval;
  counters_.clear();
}

const char *DetLogEngine::root() const {
  return root_.c_str();
}

int DetLogEngine::interval() const {
  return interval_;
}

bool DetLogEngine::should_log(int pad) {
  int counter = counters_[pad];
  bool write = interval_ == 0 || (counter % interval_ == 0);
  if (write) {
    counter = 0;
  }
  counters_[pad] = counter + 1;
  return write;
}

FILE *DetLogEngine::file_for_pad(int pad, bool *ok) {
  FILE *handle = nullptr;
  auto it = files_.find(pad);
  if (it != files_.end()) {
    handle = it->second;
  }
  if (handle == nullptr) {
    g_mkdir_with_parents(root_.c_str(), 0755);
    char path[512];
    std::snprintf(path, sizeof(path), "%s/probe_%d.log", root_.c_str(), pad);
    struct stat info {};
    bool empty = stat(path, &info) != 0 || info.st_size == 0;
    handle = fopen(path, "a");
    *ok = handle != nullptr;
    if (handle != nullptr) {
      files_[pad] = handle;
      if (empty) {
        fputs(NVLOGGER_LOG_HEADER, handle);
        fflush(handle);
      }
    }
  } else {
    *ok = true;
  }
  return handle;
}

bool DetLogEngine::write_line(int pad, const std::string &line) {
  bool ok = true;
  FILE *handle = file_for_pad(pad, &ok);
  if (!ok || handle == nullptr) {
    ok = false;
  } else {
    if (fseek(handle, 0, SEEK_END) != 0) {
      ok = false;
    } else {
      long pos = ftell(handle);
      size_t need = line.size() + 1;
      if (pos >= 0 && static_cast<size_t>(pos) + need >= NVLOGGER_MAX_BYTES) {
        if (fflush(handle) != 0 || ftruncate(fileno(handle), 0) != 0 ||
            fseek(handle, 0, SEEK_SET) != 0) {
          ok = false;
        } else {
          fputs(NVLOGGER_LOG_HEADER, handle);
        }
      }
    }
    if (ok) {
      if (fputs(line.c_str(), handle) == EOF || fputc('\n', handle) == EOF ||
          fflush(handle) != 0) {
        ok = false;
      }
    }
  }
  return ok;
}

std::string DetLogEngine::escape_label(const char *label) const {
  std::string out;
  const char *src = label != nullptr ? label : "";
  for (const char *p = src; *p != '\0'; ++p) {
    char ch = *p;
    if (ch == '\\' || ch == '"') {
      out.push_back('\\');
    }
    if (ch != '\n' && ch != '\r') {
      out.push_back(ch);
    }
  }
  return out;
}

std::string DetLogEngine::build_line(NvDsFrameMeta *frame_meta) const {
  std::ostringstream json;
  json << "{\"pad\":" << static_cast<int>(frame_meta->pad_index)
       << ",\"source\":" << static_cast<int>(frame_meta->source_id)
       << ",\"frame\":" << static_cast<int>(frame_meta->frame_num)
       << ",\"object\":[";
  bool first = true;
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *object_meta = static_cast<NvDsObjectMeta *>(item->data);
    if (object_meta == nullptr) {
      continue;
    }
    if (!first) {
      json << ",";
    }
    first = false;
    float left = object_meta->rect_params.left;
    float top = object_meta->rect_params.top;
    int x1 = round_coord(left);
    int y1 = round_coord(top);
    int x2 = round_coord(left + object_meta->rect_params.width);
    int y2 = round_coord(top + object_meta->rect_params.height);
    double conf = std::round(static_cast<double>(object_meta->confidence) * 100.0) / 100.0;
    int32_t object_id = -1;
    if (object_meta->object_id != kUntrackedObjectId) {
      object_id = static_cast<int32_t>(object_meta->object_id);
    }
    json << "[" << x1 << "," << y1 << "," << x2 << "," << y2 << ","
         << std::fixed << std::setprecision(2) << conf << ","
         << object_meta->class_id << ",\"" << escape_label(object_meta->obj_label)
         << "\"," << object_id << "]";
  }
  json << "]}";
  return json.str();
}

bool DetLogEngine::process_frame(NvDsFrameMeta *frame_meta) {
  bool ok = true;
  int pad = static_cast<int>(frame_meta->pad_index);
  if (should_log(pad)) {
    ok = write_line(pad, build_line(frame_meta));
  }
  return ok;
}

}  // namespace nvlogger
