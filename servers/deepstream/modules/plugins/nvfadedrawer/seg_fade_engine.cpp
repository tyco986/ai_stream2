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
  obj->mask_params.width = 0;
  obj->mask_params.height = 0;
  obj->mask_params.threshold = 0.0f;
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

}  // namespace nvfadedrawer
