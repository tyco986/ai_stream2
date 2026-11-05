#pragma once

#include <gst/gst.h>

#include "nvdsmeta.h"

namespace nvstgcnppactionlabel {

class ActionLabelEngine {
 public:
  ActionLabelEngine();
  void set_classifier_unique_id(int classifier_unique_id);
  int classifier_unique_id() const;
  void process_buffer(GstBuffer *buffer);

 private:
  gboolean object_ready(NvDsObjectMeta *obj) const;
  void classifier_result(
      NvDsObjectMeta *obj,
      const char **name,
      float *score,
      gint *class_id) const;
  void write_action_meta(
      NvDsBatchMeta *batch_meta,
      NvDsObjectMeta *obj,
      float conf,
      gint class_id,
      gboolean ready,
      const char *label);
  void write_object(NvDsBatchMeta *batch_meta, NvDsObjectMeta *obj);

  int classifier_unique_id_;
};

}  // namespace nvstgcnppactionlabel
