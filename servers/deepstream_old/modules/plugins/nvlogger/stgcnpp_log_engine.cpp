#include "stgcnpp_log_engine.hpp"

#include "nvds_stgcnpp_action_meta.h"
#include "nvds_stgcnpp_ready_meta.h"

#include <cmath>
#include <iomanip>

namespace nvlogger {

namespace {

constexpr char kStgcnppLogHeader[] =
    "# object item: {det:[x1, y1, x2, y2, conf, cls, label, id],"
    "action:[action_conf, action_cls, action_label, ready, length]}\n"
    "# line: {ts, pad, source, frame, latency, num, object}\n";

}  // namespace

const char *StgcnppLogEngine::log_header() const
{
  return kStgcnppLogHeader;
}

void StgcnppLogEngine::append_object_item(
    std::ostringstream &json,
    NvDsObjectMeta *object_meta) const
{
  float conf = NVDS_STGCNPP_ACTION_CONF_NONE;
  gint class_id = NVDS_STGCNPP_ACTION_CLASS_NONE;
  const char *label = "";
  gboolean ready = FALSE;
  gint length = 0;
  NvDsStgcnppActionMeta *meta = nvds_stgcnpp_action_meta_from_obj(object_meta);
  if (meta != nullptr) {
    conf = meta->conf;
    class_id = meta->class_id;
    label = meta->label;
    ready = meta->ready;
  }
  NvDsStgcnppReadyMeta *ready_meta = nvds_stgcnpp_ready_meta_from_obj(object_meta);
  if (ready_meta != nullptr) {
    length = ready_meta->length;
  }
  double logged = std::round(static_cast<double>(conf) * 100.0) / 100.0;
  json << "{\"det\":[";
  append_det_fields(json, object_meta);
  json << "],\"action\":[" << std::fixed << std::setprecision(2) << logged << "," << class_id
       << ",\"" << escape_label(label) << "\"," << (ready ? "true" : "false") << "," << length
       << "]}";
}

}  // namespace nvlogger
