#pragma once

#include "det_log_engine.hpp"
#include "nvds_presence_event_meta.h"

#define NVLOGGER_PRESENCE_LOG_HEADER \
  "# object item: [x1, y1, x2, y2, conf, cls, label, id]\n" \
  "# line: {ts, pad, source, frame, latency, num, eventcode, object}\n"

namespace nvlogger {

class PresenceLogEngine : public DetLogEngine {
 public:
  PresenceLogEngine();
  ~PresenceLogEngine() override;

  PresenceLogEngine(const PresenceLogEngine &) = delete;
  PresenceLogEngine &operator=(const PresenceLogEngine &) = delete;

 protected:
  const char *log_header() const override;
  std::string build_line(NvDsFrameMeta *frame_meta, double latency_ms) const override;

 private:
  const NvDsPresenceEventMeta *find_event_meta(NvDsFrameMeta *frame_meta) const;
  std::string eventcode_field(NvDsFrameMeta *frame_meta) const;
};

}  // namespace nvlogger
