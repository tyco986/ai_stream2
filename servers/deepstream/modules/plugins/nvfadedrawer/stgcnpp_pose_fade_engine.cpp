#include "stgcnpp_pose_fade_engine.hpp"

namespace nvfadedrawer {

namespace {

constexpr int kDefaultClassifierUniqueId = 4;

}  // namespace

StgcnppPoseFadeEngine::StgcnppPoseFadeEngine()
    : classifier_unique_id_(kDefaultClassifierUniqueId)
{
}

void StgcnppPoseFadeEngine::set_classifier_unique_id(int classifier_unique_id)
{
  classifier_unique_id_ = classifier_unique_id;
}

int StgcnppPoseFadeEngine::classifier_unique_id() const
{
  return classifier_unique_id_;
}

void StgcnppPoseFadeEngine::read_action(
    NvDsObjectMeta *obj,
    const char **action_name,
    float *action_conf) const
{
  const char *name = "";
  float conf = 0.0f;
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
          name = info->pResult_label;
        } else {
          name = info->result_label;
        }
        conf = info->result_prob;
      }
    }
  }
  if (action_name != nullptr) {
    *action_name = name;
  }
  if (action_conf != nullptr) {
    *action_conf = conf;
  }
}

void StgcnppPoseFadeEngine::write_label(NvDsObjectMeta *obj) const
{
  const char *action_name = "";
  float action_conf = 0.0f;
  if (obj != nullptr) {
    read_action(obj, &action_name, &action_conf);
    fill_action_label(
        obj,
        action_name,
        action_conf,
        obj->confidence,
        track_display_id(obj->object_id));
  }
}

}  // namespace nvfadedrawer
