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
  int interval() const;
  int fade_time() const;
  bool show_label() const;

  virtual void process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta);
  virtual void drop_pad(int pad_index);

 protected:
  struct CachedMask {
    std::vector<float> data;
    unsigned int size = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    float threshold = 0.0f;
  };

  struct CachedObject {
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float confidence = 0.0f;
    int class_id = 0;
    std::uint64_t object_id = kUntrackedObjectId;
    std::string label;
    CachedMask mask;
  };

  struct TrackCache {
    float confidence = 0.0f;
    CachedMask mask;
  };

  struct StreamState {
    int phase = 0;
    std::vector<CachedObject> objects;
    std::unordered_map<std::uint64_t, TrackCache> tracks;
  };

  Rgba resolve_box_color(NvDsFrameMeta *frame_meta) const;
  Rgba fade_color(const float color[4], float alpha) const;
  StreamState &state_for(int pad_index);
  void apply_style(
      NvDsObjectMeta *obj,
      const Rgba &box_color,
      const Rgba &text_color,
      const Rgba &text_bg_color) const;
  virtual void write_label(NvDsObjectMeta *obj) const;
  void apply_label_text(NvDsObjectMeta *obj, const char *line) const;
  std::int64_t track_display_id(std::uint64_t object_id) const;
  virtual void decorate_object(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      NvDsObjectMeta *obj,
      float fade_alpha);
  virtual void decorate_cached(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      const CachedObject &cached,
      float fade_alpha);
  void set_rect_color(NvOSD_RectParams *rect, const Rgba &color) const;
  void copy_mask(
      CachedMask &cached,
      const float *data,
      unsigned int mask_size,
      unsigned int mask_width,
      unsigned int mask_height,
      float mask_threshold) const;
  CachedObject cache_object(const NvDsObjectMeta *obj) const;
  CachedObject cache_object(const NvDsBboxSnapshotBox &box) const;

  std::vector<float> alpha_lut_;

 private:
  void rebuild_lut();
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
  void fill_label(NvDsObjectMeta *obj, const char *label, float conf) const;
  void restore_object(NvDsObjectMeta *obj, const CachedObject &cached) const;
  void clear_label(NvDsObjectMeta *obj) const;

  int interval_ = 0;
  int fade_time_ = 0;
  bool show_label_ = false;
  std::unordered_map<int, StreamState> streams_;
};

class DetFadeEngineWithTracker : public virtual DetFadeEngine {
 public:
  void set_show_snap(bool show_snap);
  bool show_snap() const;
  void process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta) override;

 protected:
  void write_label(NvDsObjectMeta *obj) const override;

 private:
  const NvDsBboxSnapshotMeta *find_snapshot_meta(NvDsFrameMeta *frame_meta) const;
  void snapshot_objects(StreamState &state, const NvDsBboxSnapshotMeta *snapshot) const;
  void apply_shadow_style(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta);
  void apply_snap_style(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      const StreamState &state,
      float fade_alpha,
      const Rgba &box_color);
  void cache_live_tracks(StreamState &state, NvDsFrameMeta *frame_meta) const;
  void apply_track_cache(NvDsObjectMeta *obj, const StreamState &state) const;

  bool show_snap_ = true;
};

}  // namespace nvfadedrawer
