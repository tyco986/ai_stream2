#pragma once

#include "fade_core.hpp"

namespace nvfadedrawer {

class SegFadeEngine : public DetFadeEngine {
 public:
  void set_show_mask(bool show_mask);
  bool show_mask() const;

 protected:
  void decorate_object(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      NvDsObjectMeta *obj,
      float fade_alpha) override;

 private:
  void clear_mask(NvDsObjectMeta *obj) const;

  bool show_mask_ = true;
};

}  // namespace nvfadedrawer
