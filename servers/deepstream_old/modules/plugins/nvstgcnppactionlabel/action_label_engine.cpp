#include "action_label_engine.hpp"

#include "constants.hpp"
#include "gstnvdsmeta.h"
#include "nvds_stgcnpp_action_meta.h"
#include "nvds_stgcnpp_ready_meta.h"

namespace nvstgcnppactionlabel {

ActionLabelEngine::ActionLabelEngine()
    : classifier_unique_id_(kDefaultClassifierUniqueId)
{
}

void ActionLabelEngine::set_classifier_unique_id(int classifier_unique_id)
{
  classifier_unique_id_ = classifier_unique_id;
}

int ActionLabelEngine::classifier_unique_id() const
{
  return classifier_unique_id_;
}

gboolean ActionLabelEngine::object_ready(NvDsObjectMeta *obj) const
{
  gboolean ready = FALSE;
  if (obj != nullptr) {
    for (NvDsMetaList *item = obj->obj_user_meta_list; item != nullptr; item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta == nullptr ||
          user_meta->base_meta.meta_type != NVDS_STGCNPP_READY_USER_META) {
        continue;
      }
      auto *meta = static_cast<NvDsStgcnppReadyMeta *>(user_meta->user_meta_data);
      if (meta != nullptr) {
        ready = meta->ready;
      }
    }
  }
  return ready;
}

void ActionLabelEngine::classifier_result(
    NvDsObjectMeta *obj,
    const char **name,
    float *score,
    gint *class_id) const
{
  const char *label = nullptr;
  float conf = kActionConfNone;
  gint cls = kActionClassNone;
  if (obj != nullptr) {
    for (NvDsMetaList *item = obj->classifier_meta_list; item != nullptr; item = item->next) {
      auto *classifier = static_cast<NvDsClassifierMeta *>(item->data);
      if (classifier == nullptr ||
          static_cast<int>(classifier->unique_component_id) != classifier_unique_id_) {
        continue;
      }
      for (NvDsMetaList *label_item = classifier->label_info_list; label_item != nullptr;
           label_item = label_item->next) {
        auto *info = static_cast<NvDsLabelInfo *>(label_item->data);
        if (info == nullptr) {
          continue;
        }
        if (info->pResult_label != nullptr && info->pResult_label[0] != '\0') {
          label = info->pResult_label;
        } else {
          label = info->result_label;
        }
        conf = info->result_prob;
        cls = static_cast<gint>(info->result_class_id);
      }
    }
  }
  if (name != nullptr) {
    *name = label;
  }
  if (score != nullptr) {
    *score = conf;
  }
  if (class_id != nullptr) {
    *class_id = cls;
  }
}

void ActionLabelEngine::write_action_meta(
    NvDsBatchMeta *batch_meta,
    NvDsObjectMeta *obj,
    float conf,
    gint class_id,
    gboolean ready,
    const char *label)
{
  NvDsStgcnppActionMeta *existing = nvds_stgcnpp_action_meta_from_obj(obj);
  NvDsStgcnppActionMeta *meta = existing;
  if (meta == nullptr) {
    NvDsUserMeta *user_meta = nvds_acquire_user_meta_from_pool(batch_meta);
    if (user_meta != nullptr) {
      meta = static_cast<NvDsStgcnppActionMeta *>(g_malloc0(sizeof(NvDsStgcnppActionMeta)));
      user_meta->user_meta_data = meta;
      user_meta->base_meta.meta_type = NVDS_STGCNPP_ACTION_USER_META;
      user_meta->base_meta.copy_func = nvds_stgcnpp_action_meta_copy;
      user_meta->base_meta.release_func = nvds_stgcnpp_action_meta_release;
      nvds_add_user_meta_to_obj(obj, user_meta);
    }
  }
  if (meta != nullptr) {
    meta->conf = conf;
    meta->class_id = class_id;
    meta->ready = ready;
    g_strlcpy(meta->label, label != nullptr ? label : "", sizeof(meta->label));
  }
}

void ActionLabelEngine::write_object(NvDsBatchMeta *batch_meta, NvDsObjectMeta *obj)
{
  gboolean ready = object_ready(obj);
  const char *cls = nullptr;
  float cls_conf = kActionConfNone;
  gint cls_id = kActionClassNone;
  if (ready) {
    classifier_result(obj, &cls, &cls_conf, &cls_id);
  }
  const char *label = kLabelNa;
  float conf = kActionConfNone;
  gint class_id = kActionClassNone;
  if (ready) {
    label = (cls != nullptr && cls[0] != '\0') ? cls : kLabelUnknown;
    conf = (cls != nullptr && cls[0] != '\0') ? cls_conf : kActionConfNone;
    class_id = (cls != nullptr && cls[0] != '\0') ? cls_id : kActionClassNone;
  }
  write_action_meta(batch_meta, obj, conf, class_id, ready, label);
}

void ActionLabelEngine::process_buffer(GstBuffer *buffer)
{
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buffer);
  if (batch_meta != nullptr) {
    nvds_acquire_meta_lock(batch_meta);
    for (NvDsMetaList *frame_item = batch_meta->frame_meta_list; frame_item != nullptr;
         frame_item = frame_item->next) {
      auto *frame_meta = static_cast<NvDsFrameMeta *>(frame_item->data);
      if (frame_meta == nullptr) {
        continue;
      }
      for (NvDsMetaList *obj_item = frame_meta->obj_meta_list; obj_item != nullptr;
           obj_item = obj_item->next) {
        auto *object_meta = static_cast<NvDsObjectMeta *>(obj_item->data);
        if (object_meta != nullptr) {
          write_object(batch_meta, object_meta);
        }
      }
    }
    nvds_release_meta_lock(batch_meta);
  }
}

}  // namespace nvstgcnppactionlabel
