#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "gstnvdsmeta.h"
#include "nvdsmeta.h"
#include "nvfadedrawer_constants.h"

namespace nvfadedrawer {

struct Rgba {
  float r;
  float g;
  float b;
  float a;
};

class DetFadeEngine {
 public:
  DetFadeEngine();
  virtual ~DetFadeEngine() = default;

  void set_interval(int interval);
  void set_fade_time(int fade_time);
  void set_show_label(bool show_label);
  int interval() const;
  int fade_time() const;
  bool show_label() const;

  virtual void process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta);
  virtual void drop_pad(int pad_index);

 protected:
  struct StreamState {
    int phase = 0;
  };

  bool is_inference_frame(unsigned int frame_num) const;
  Rgba resolve_box_color(NvDsFrameMeta *frame_meta) const;
  Rgba fade_color(const float color[4], float alpha) const;
  StreamState &state_for(int pad_index);
  void apply_style(
      NvDsObjectMeta *obj,
      const Rgba &box_color,
      const Rgba &text_color,
      const Rgba &text_bg_color) const;
  virtual void decorate_object(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      NvDsObjectMeta *obj,
      float fade_alpha);

 private:
  void rebuild_lut();
  void fill_label(NvDsObjectMeta *obj, const char *label, float conf, std::int64_t track_id) const;
  void set_rect_color(NvOSD_RectParams *rect, const Rgba &color) const;
  void clear_label(NvDsObjectMeta *obj) const;

  int interval_ = 0;
  int fade_time_ = 0;
  bool show_label_ = false;
  std::vector<float> alpha_lut_;
  std::unordered_map<int, StreamState> streams_;
};

}  // namespace nvfadedrawer
