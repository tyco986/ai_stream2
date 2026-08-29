#include "presence_log_engine.hpp"

#include "nvds_presence_event_meta.h"

namespace nvlogger {

PresenceLogEngine::PresenceLogEngine() {}

PresenceLogEngine::~PresenceLogEngine() {}

const NvDsPresenceEventMeta *PresenceLogEngine::find_event_meta(
    NvDsFrameMeta *frame_meta) const
{
  const NvDsPresenceEventMeta *event = nullptr;
  if (frame_meta != nullptr) {
    for (NvDsMetaList *item = frame_meta->frame_user_meta_list; item != nullptr;
         item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta != nullptr &&
          user_meta->base_meta.meta_type == NVDS_PRESENCE_EVENT_USER_META) {
        event = static_cast<const NvDsPresenceEventMeta *>(user_meta->user_meta_data);
      }
    }
  }
  return event;
}

std::string PresenceLogEngine::eventcode_field(NvDsFrameMeta *frame_meta) const {
  std::string field = ",\"eventcode\":";
  const NvDsPresenceEventMeta *event = find_event_meta(frame_meta);
  if (event == nullptr) {
    field += "null";
  } else {
    guint n = event->num_classes;
    if (n > NVDS_PRESENCE_EVENT_CODE_LEN) {
      n = NVDS_PRESENCE_EVENT_CODE_LEN;
    }
    field += "\"";
    for (guint i = 0; i < n; i++) {
      field.push_back(event->event_codes[i]);
    }
    field += "\"";
  }
  return field;
}

const char *PresenceLogEngine::log_header() const {
  return NVLOGGER_PRESENCE_LOG_HEADER;
}

std::string PresenceLogEngine::build_line(NvDsFrameMeta *frame_meta, double latency_ms) const {
  std::string line = DetLogEngine::build_line(frame_meta, latency_ms);
  const char *marker = ",\"object\":";
  size_t pos = line.find(marker);
  if (pos != std::string::npos) {
    line.insert(pos, eventcode_field(frame_meta));
  }
  return line;
}

}  // namespace nvlogger
