#include "plugin.h"
#include "custom_factory.hpp"
#include "common_factory.hpp"
#include "latency_probe.hpp"

#include <cstdio>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <gst/gst.h>

#include "gstnvdsmeta.h"
#include "nvds_latency_meta.h"
#include "nvdsmeta.h"

using namespace deepstream;

namespace {

constexpr std::size_t kMaxStoreEntries = 8192;

uint64_t latency_key(unsigned int source_id, unsigned int frame_num) {
  return (static_cast<uint64_t>(source_id) << 32) | static_cast<uint64_t>(frame_num);
}

/* Logical keys from configs/generator sink_path.yml (index suffix stripped). */
constexpr const char* kLogicalNames[] = {
    "appsink_raw",
    "appsink_vis",
    "capsfilter_osd",
    "capsfilter_raw",
    "capsfilter_vis",
    "filesink",
    "h264parse",
    "mp4mux",
    "nvdsanalytics",
    "nvjpegenc",
    "nvosdbin",
    "nvsahipostprocess",
    "nvsahipreprocess",
    "nvstreamdemux",
    "nvstreammux",
    "nvtracker",
    "nvurisrcbin",
    "nvv4l2h264enc",
    "nvvideoconvert",
    "nvvideoconvert_osd",
    "nvvideoconvert_raw",
    "nvvideoconvert_vis",
    "pgie",
    "queue_demux",
    "queue_enc",
    "queue_osd",
    "queue_raw",
    "queue_sahi",
    "queue_vis",
    "rtspclientsink",
    "sgie0",
    "sgie1",
    "tee_raw",
    "tee_vis",
};

/* Keys are interned pointers from kLogicalNames; pointer identity is enough. */
struct InternedCStrHash {
  std::size_t operator()(const char* value) const noexcept {
    return std::hash<const void*>{}(static_cast<const void*>(value));
  }
};

struct InternedCStrEq {
  bool operator()(const char* left, const char* right) const noexcept {
    return left == right;
  }
};

using ComponentMap = std::unordered_map<const char*, double, InternedCStrHash, InternedCStrEq>;

bool contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

const char* lookup_logical(std::string_view text) {
  const char* logical = nullptr;
  for (const char* name : kLogicalNames) {
    if (text == name) {
      logical = name;
      break;
    }
  }
  return logical;
}

std::string_view strip_trailing_digits(std::string_view text) {
  while (!text.empty() && text.back() >= '0' && text.back() <= '9') {
    text.remove_suffix(1);
  }
  return text;
}

/* Longest kLogicalNames that is a prefix of text (nvstreammux-nvstreammux → nvstreammux). */
const char* lookup_longest_prefix(std::string_view text) {
  const char* best = nullptr;
  std::size_t best_len = 0;
  for (const char* name : kLogicalNames) {
    std::string_view logical(name);
    if (logical.size() <= best_len || text.size() < logical.size()) {
      continue;
    }
    if (text.substr(0, logical.size()) != logical) {
      continue;
    }
    if (text.size() > logical.size()) {
      const char next = text[logical.size()];
      if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') ||
          (next >= '0' && next <= '9')) {
        continue;
      }
    }
    best = name;
    best_len = logical.size();
  }
  return best;
}

/* DS bin/internal stamps that do not equal pipeline element names. */
const char* lookup_alias(std::string_view text) {
  const char* logical = nullptr;
  if (contains(text, "nvv4l2decoder") || contains(text, "audiodecoder") || text == "src") {
    logical = lookup_logical("nvurisrcbin");
  } else if (contains(text, "nvosd")) {
    logical = lookup_logical("nvosdbin");
  } else {
    logical = lookup_longest_prefix(text);
  }
  return logical;
}

/* Exact / strip index / DS aliases; unknown -> nullptr. */
const char* normalize_component_name(const char* name) {
  const char* logical = nullptr;
  if (name != nullptr && name[0] != '\0') {
    std::string_view text(name);
    logical = lookup_logical(text);
    if (logical == nullptr) {
      logical = lookup_logical(strip_trailing_digits(text));
    }
    if (logical == nullptr) {
      logical = lookup_alias(text);
    }
  }
  return logical;
}

struct LatencyRecord {
  double latency_ms = -1.0;
  ComponentMap components;
};

struct GstBufferUnref {
  void operator()(GstBuffer* buffer) const {
    if (buffer != nullptr) {
      gst_buffer_unref(buffer);
    }
  }
};

class LatencyStore {
 public:
  static LatencyStore& instance() {
    static LatencyStore store;
    return store;
  }

  void merge(std::unordered_map<uint64_t, LatencyRecord>&& batch) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& item : batch) {
      LatencyRecord& dst = table_[item.first];
      if (item.second.latency_ms >= 0.0) {
        dst.latency_ms = item.second.latency_ms;
      }
      for (auto& comp : item.second.components) {
        dst.components[comp.first] = comp.second;
      }
    }
    trim_locked();
  }

  bool take(
      unsigned int source_id,
      unsigned int frame_num,
      double* latency_ms,
      char names[][kMaxComponentName],
      double* values,
      int max_components,
      int* count) {
    bool found = false;
    int written = 0;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_.find(latency_key(source_id, frame_num));
    if (it != table_.end()) {
      if (latency_ms != nullptr) {
        *latency_ms = it->second.latency_ms;
      }
      if (names != nullptr && values != nullptr && max_components > 0) {
        for (const auto& item : it->second.components) {
          if (written >= max_components) {
            break;
          }
          std::snprintf(names[written], kMaxComponentName, "%s", item.first);
          values[written] = item.second;
          written += 1;
        }
      }
      table_.erase(it);
      found = true;
    } else if (latency_ms != nullptr) {
      *latency_ms = -1.0;
    }
    if (count != nullptr) {
      *count = written;
    }
    return found;
  }

 private:
  LatencyStore() = default;

  void trim_locked() {
    if (table_.size() <= kMaxStoreEntries) {
      return;
    }
    const std::size_t target = kMaxStoreEntries / 2;
    auto it = table_.begin();
    while (table_.size() > target && it != table_.end()) {
      it = table_.erase(it);
    }
  }

  std::mutex mutex_;
  std::unordered_map<uint64_t, LatencyRecord> table_;
};

void collect_components_into(NvDsUserMetaList* list, ComponentMap* components) {
  for (NvDsMetaList* item = list; item != nullptr; item = item->next) {
    auto* user_meta = static_cast<NvDsUserMeta*>(item->data);
    if (user_meta == nullptr ||
        user_meta->base_meta.meta_type != NVDS_LATENCY_MEASUREMENT_META) {
      continue;
    }
    auto* comp = static_cast<NvDsMetaCompLatency*>(user_meta->user_meta_data);
    if (comp == nullptr) {
      continue;
    }
    const char* logical = normalize_component_name(comp->component_name);
    if (logical == nullptr) {
      continue;
    }
    double component_ms = comp->out_system_timestamp - comp->in_system_timestamp;
    if (component_ms < 0.0) {
      continue;
    }
    (*components)[logical] = component_ms;
  }
}

void collect_batch_records(
    GstBuffer* gst_buf,
    std::unordered_map<uint64_t, LatencyRecord>* records) {
  NvDsBatchMeta* batch_meta = gst_buffer_get_nvds_batch_meta(gst_buf);
  if (batch_meta == nullptr) {
    return;
  }

  ComponentMap batch_components;
  collect_components_into(batch_meta->batch_user_meta_list, &batch_components);

  for (NvDsMetaList* frame_item = batch_meta->frame_meta_list;
       frame_item != nullptr;
       frame_item = frame_item->next) {
    auto* frame_meta = static_cast<NvDsFrameMeta*>(frame_item->data);
    if (frame_meta == nullptr) {
      continue;
    }
    LatencyRecord& record =
        (*records)[latency_key(frame_meta->source_id, frame_meta->frame_num)];
    collect_components_into(frame_meta->frame_user_meta_list, &record.components);
    for (const auto& item : batch_components) {
      record.components[item.first] = item.second;
    }
  }
}

}  // namespace

probeReturn NvDsLatencyProbe::handleBuffer(BufferProbe& probe, const Buffer& buffer) {
  (void)probe;
  std::unordered_map<uint64_t, LatencyRecord> records;

  /* Copy + give() transfers one GstBuffer ref; RAII unref prevents leaks. */
  Buffer owned(buffer);
  std::unique_ptr<GstBuffer, GstBufferUnref> gst_buf(owned.give());
  if (gst_buf) {
    collect_batch_records(gst_buf.get(), &records);
  }

  for (auto& latency : buffer.measureLatency()) {
    records[latency_key(latency.source_id, latency.frame_num)].latency_ms = latency.latency;
  }
  if (!records.empty()) {
    LatencyStore::instance().merge(std::move(records));
  }
  return probeReturn::Probe_Ok;
}

#define FACTORY_NAME "latency_probe"

DS_CUSTOM_PLUGIN_DEFINE(
    latency_probe,
    "Custom probe to measure end-to-end and component latency",
    "0.8",
    "Proprietary")

DS_CUSTOM_FACTORY_DEFINE(
  FACTORY_NAME,
  "Latency measurement probe with in-process multi-component lookup",
  "probe",
  "latency measurement probe exposing latency_probe_take()",
  "ai_stream2",
  BufferProbe,
  NvDsLatencyProbe
)

extern "C" int latency_probe_take(
    unsigned int source_id,
    unsigned int frame_num,
    double* latency_ms,
    char names[][kMaxComponentName],
    double* values,
    int max_components,
    int* count) {
  return LatencyStore::instance().take(
             source_id, frame_num, latency_ms, names, values, max_components, count)
             ? 1
             : 0;
}
