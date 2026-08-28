#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "gstnvdsmeta.h"
#include "nvds_bbox_snapshot_meta.h"
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
  void set_show_snap(bool show_snap);
  int interval() const;
  int fade_time() const;
  bool show_label() const;
  bool show_snap() const;

  virtual void process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta);
  virtual void drop_pad(int pad_index);

 protected:
  struct CachedObject {
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float confidence = 0.0f;
    int class_id = 0;
    std::uint64_t object_id = kUntrackedObjectId;
    std::string label;
    std::vector<float> mask;
    unsigned int mask_size = 0;
    unsigned int mask_width = 0;
    unsigned int mask_height = 0;
    float mask_threshold = 0.0f;
  };

  struct StreamState {
    int phase = 0;
    std::vector<CachedObject> objects;
  };

  const NvDsBboxSnapshotMeta *find_snapshot_meta(NvDsFrameMeta *frame_meta) const;
  Rgba resolve_box_color(NvDsFrameMeta *frame_meta) const;
  Rgba fade_color(const float color[4], float alpha) const;
  StreamState &state_for(int pad_index);
  void apply_style(
      NvDsObjectMeta *obj,
      const Rgba &box_color,
      const Rgba &text_color,
      const Rgba &text_bg_color) const;
  virtual void write_label(NvDsObjectMeta *obj) const;
  void fill_action_label(
      NvDsObjectMeta *obj,
      const char *action_name,
      float action_conf,
      float person_conf,
      std::int64_t track_id) const;
  std::int64_t track_display_id(std::uint64_t object_id) const;
  virtual void decorate_object(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      NvDsObjectMeta *obj,
      float fade_alpha);
  void process_tracker_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta);

 private:
  void rebuild_lut();
  void snapshot_objects(StreamState &state, const NvDsBboxSnapshotMeta *snapshot) const;
  void snapshot_frame_objects(StreamState &state, NvDsFrameMeta *frame_meta) const;
  void style_frame_objects(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      float fade_alpha,
      const Rgba &box_color,
      const Rgba &text_color,
      const Rgba &text_bg_color);
  void inject_inferred_objects(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      const StreamState &state,
      float fade_alpha,
      const Rgba &box_color,
      const Rgba &text_color,
      const Rgba &text_bg_color);
  void style_tracker_objects(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta);
  void hide_tracker_visuals(NvDsFrameMeta *frame_meta);
  void fill_label(NvDsObjectMeta *obj, const char *label, float conf, std::int64_t track_id) const;
  void set_rect_color(NvOSD_RectParams *rect, const Rgba &color) const;
  void clear_label(NvDsObjectMeta *obj) const;
  void copy_mask(
      CachedObject &cached,
      const float *data,
      unsigned int mask_size,
      unsigned int mask_width,
      unsigned int mask_height,
      float mask_threshold) const;
  CachedObject cache_object(const NvDsObjectMeta *obj) const;
  CachedObject cache_object(const NvDsBboxSnapshotBox &box) const;
  void restore_object(NvDsObjectMeta *obj, const CachedObject &cached) const;
  virtual void hide_tracker_mask(NvDsObjectMeta *obj) const;

  int interval_ = 0;
  int fade_time_ = 0;
  bool show_label_ = false;
  bool show_snap_ = true;
  std::vector<float> alpha_lut_;
  std::unordered_map<int, StreamState> streams_;
};

class DetFadeEngineWithTracker : public DetFadeEngine {
 public:
  void process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta) override;
};

}  // namespace nvfadedrawer
