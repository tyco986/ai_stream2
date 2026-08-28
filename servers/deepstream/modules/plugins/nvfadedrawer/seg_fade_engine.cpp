#include "seg_fade_engine.hpp"

namespace nvfadedrawer {

void SegFadeEngine::set_show_mask(bool show_mask)
{
  show_mask_ = show_mask;
}

bool SegFadeEngine::show_mask() const
{
  return show_mask_;
}

void SegFadeEngine::clear_mask(NvDsObjectMeta *obj) const
{
  g_free(obj->mask_params.data);
  obj->mask_params.data = nullptr;
  obj->mask_params.size = 0;
  obj->mask_params.width = 0;
  obj->mask_params.height = 0;
  obj->mask_params.threshold = 0.0f;
}

void SegFadeEngine::hide_tracker_mask(NvDsObjectMeta *obj) const
{
  clear_mask(obj);
}

void SegFadeEngine::decorate_object(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta,
    NvDsObjectMeta *obj,
    float fade_alpha)
{
  (void)batch_meta;
  (void)frame_meta;
  (void)fade_alpha;
  if (!show_mask_) {
    clear_mask(obj);
  }
}

void SegFadeEngineWithTracker::process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  process_tracker_frame(batch_meta, frame_meta);
}

}  // namespace nvfadedrawer
