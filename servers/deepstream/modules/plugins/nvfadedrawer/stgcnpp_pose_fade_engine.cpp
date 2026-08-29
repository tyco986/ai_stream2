#include "stgcnpp_pose_fade_engine.hpp"

#include "nvds_stgcnpp_action_meta.h"

namespace nvfadedrawer {

void StgcnppPoseFadeEngine::cache_live_actions(NvDsFrameMeta *frame_meta)
{
  if (frame_meta != nullptr) {
    for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
      auto *obj = static_cast<NvDsObjectMeta *>(item->data);
      if (obj == nullptr) {
        continue;
      }
      NvDsStgcnppActionMeta *meta = nvds_stgcnpp_action_meta_from_obj(obj);
      if (meta != nullptr && meta->label[0] != '\0') {
        LastAction action;
        action.name = meta->label;
        action.conf = meta->conf;
        last_actions_[obj->object_id] = action;
      }
    }
  }
}

void StgcnppPoseFadeEngine::process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  cache_live_actions(frame_meta);
  PoseFadeEngineWithTracker::process_frame(batch_meta, frame_meta);
}

void StgcnppPoseFadeEngine::write_label(NvDsObjectMeta *obj) const
{
  if (obj != nullptr) {
    const char *name = "";
    float action_conf = NVDS_STGCNPP_ACTION_CONF_NONE;
    NvDsStgcnppActionMeta *meta = nvds_stgcnpp_action_meta_from_obj(obj);
    if (meta != nullptr && meta->label[0] != '\0') {
      name = meta->label;
      action_conf = meta->conf;
    } else {
      auto it = last_actions_.find(obj->object_id);
      if (it != last_actions_.end()) {
        name = it->second.name.c_str();
        action_conf = it->second.conf;
      }
    }
    fill_action_label(
        obj,
        name,
        action_conf,
        obj->confidence,
        track_display_id(obj->object_id));
  }
}

}  // namespace nvfadedrawer
