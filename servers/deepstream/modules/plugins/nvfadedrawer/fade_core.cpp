#include "fade_core.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <glib.h>

#include "nvds_presence_event_meta.h"

namespace nvfadedrawer {

namespace {

void copy_color(NvOSD_ColorParams *dst, const Rgba &src)
{
  dst->red = src.r;
  dst->green = src.g;
  dst->blue = src.b;
  dst->alpha = src.a;
}

std::int64_t display_id(std::uint64_t object_id)
{
  std::int64_t id = -1;
  if (object_id != kUntrackedObjectId) {
    id = static_cast<std::int64_t>(object_id);
  }
  return id;
}

const NvDsPresenceEventMeta *find_event_meta(NvDsFrameMeta *frame_meta)
{
  const NvDsPresenceEventMeta *event = nullptr;
  for (NvDsMetaList *item = frame_meta->frame_user_meta_list; item != nullptr; item = item->next) {
    auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
    if (user_meta == nullptr ||
        user_meta->base_meta.meta_type != NVDS_PRESENCE_EVENT_USER_META) {
      continue;
    }
    event = static_cast<const NvDsPresenceEventMeta *>(user_meta->user_meta_data);
    break;
  }
  return event;
}

}  // namespace

DetFadeEngine::DetFadeEngine()
{
  rebuild_lut();
}

void DetFadeEngine::set_interval(int interval)
{
  interval_ = interval < 0 ? 0 : interval;
  rebuild_lut();
}

void DetFadeEngine::set_fade_time(int fade_time)
{
  fade_time_ = fade_time < 0 ? 0 : fade_time;
  rebuild_lut();
}

void DetFadeEngine::set_show_label(bool show_label)
{
  show_label_ = show_label;
}

void DetFadeEngine::set_show_snap(bool show_snap)
{
  show_snap_ = show_snap;
}

int DetFadeEngine::interval() const
{
  return interval_;
}

int DetFadeEngine::fade_time() const
{
  return fade_time_;
}

bool DetFadeEngine::show_label() const
{
  return show_label_;
}

bool DetFadeEngine::show_snap() const
{
  return show_snap_;
}

void DetFadeEngine::drop_pad(int pad_index)
{
  streams_.erase(pad_index);
}

const NvDsBboxSnapshotMeta *DetFadeEngine::find_snapshot_meta(NvDsFrameMeta *frame_meta) const
{
  const NvDsBboxSnapshotMeta *snapshot = nullptr;
  for (NvDsMetaList *item = frame_meta->frame_user_meta_list; item != nullptr; item = item->next) {
    auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
    if (snapshot == nullptr && user_meta != nullptr &&
        user_meta->base_meta.meta_type == NVDS_BBOX_SNAPSHOT_USER_META) {
      snapshot = static_cast<const NvDsBboxSnapshotMeta *>(user_meta->user_meta_data);
    }
  }
  return snapshot;
}

Rgba DetFadeEngine::fade_color(const float color[4], float alpha) const
{
  Rgba out;
  out.r = color[0];
  out.g = color[1];
  out.b = color[2];
  out.a = alpha;
  return out;
}

Rgba DetFadeEngine::resolve_box_color(NvDsFrameMeta *frame_meta) const
{
  Rgba color;
  color.r = kColorGreen[0];
  color.g = kColorGreen[1];
  color.b = kColorGreen[2];
  color.a = kColorGreen[3];
  const NvDsPresenceEventMeta *event = find_event_meta(frame_meta);
  if (event != nullptr) {
    bool has_alert = false;
    bool has_transit = false;
    guint n = event->num_classes;
    if (n > NVDS_PRESENCE_EVENT_CODE_LEN) {
      n = NVDS_PRESENCE_EVENT_CODE_LEN;
    }
    for (guint i = 0; i < n; i++) {
      char code = event->event_codes[i];
      if (code == '1') {
        has_alert = true;
      } else if (code == '2') {
        has_transit = true;
      }
    }
    const float *src = kColorGreen;
    if (has_alert) {
      src = kColorRed;
    } else if (has_transit) {
      src = kColorYellow;
    }
    color.r = src[0];
    color.g = src[1];
    color.b = src[2];
    color.a = src[3];
  }
  return color;
}

DetFadeEngine::StreamState &DetFadeEngine::state_for(int pad_index)
{
  return streams_[pad_index];
}

void DetFadeEngine::rebuild_lut()
{
  alpha_lut_.clear();
  if (fade_time_ <= 0) {
    alpha_lut_.push_back(1.0f);
  } else {
    int period = interval_ > 0 ? interval_ : 1;
    int mid = period / 2;
    int tail = period - mid;
    std::vector<float> triangle;
    triangle.reserve(static_cast<std::size_t>(period));
    for (int i = 0; i < period; i++) {
      float t = 0.0f;
      if (i <= mid) {
        t = mid > 0 ? 1.0f - static_cast<float>(i) / static_cast<float>(mid) : 1.0f;
      } else {
        t = tail > 0 ? static_cast<float>(i - mid) / static_cast<float>(tail) : 0.0f;
      }
      float alpha = kMinAlpha + (1.0f - kMinAlpha) * t;
      triangle.push_back(std::round(alpha * 100.0f) / 100.0f);
    }
    for (int index = 0; index < fade_time_; index++) {
      if (index == 0) {
        alpha_lut_.insert(alpha_lut_.end(), triangle.begin(), triangle.end());
      } else if (triangle.size() > 1) {
        alpha_lut_.insert(alpha_lut_.end(), triangle.begin() + 1, triangle.end());
      }
    }
  }
  if (alpha_lut_.empty()) {
    alpha_lut_.push_back(1.0f);
  }
}

void DetFadeEngine::set_rect_color(NvOSD_RectParams *rect, const Rgba &color) const
{
  copy_color(&rect->border_color, color);
  rect->border_width = kBoxWidth;
  rect->has_bg_color = 0;
}

void DetFadeEngine::clear_label(NvDsObjectMeta *obj) const
{
  NvOSD_TextParams *text = &obj->text_params;
  g_free(text->display_text);
  text->display_text = nullptr;
  text->set_bg_clr = 0;
}

std::int64_t DetFadeEngine::track_display_id(std::uint64_t object_id) const
{
  return display_id(object_id);
}

void DetFadeEngine::fill_action_label(
    NvDsObjectMeta *obj,
    const char *action_name,
    float action_conf,
    float person_conf,
    std::int64_t track_id) const
{
  NvOSD_TextParams *text = &obj->text_params;
  NvOSD_RectParams *rect = &obj->rect_params;
  if (!show_label_) {
    clear_label(obj);
  } else {
    char line[256];
    const char *name = action_name != nullptr ? action_name : "";
    std::snprintf(
        line,
        sizeof(line),
        "%s%c%.2f%.2f%c%lld",
        name,
        kLabelSep,
        action_conf,
        person_conf,
        kLabelSep,
        static_cast<long long>(track_id));
    g_free(text->display_text);
    text->display_text = g_strdup(line);
    text->x_offset = static_cast<unsigned int>(rect->left);
    int y = static_cast<int>(rect->top) - kLabelYOffset;
    text->y_offset = y < 0 ? 0u : static_cast<unsigned int>(y);
    text->font_params.font_name = const_cast<char *>(kFontName);
    text->font_params.font_size = kFontSize;
    text->set_bg_clr = 1;
  }
}

void DetFadeEngine::write_label(NvDsObjectMeta *obj) const
{
  fill_label(obj, obj->obj_label, obj->confidence, track_display_id(obj->object_id));
}

void DetFadeEngine::fill_label(
    NvDsObjectMeta *obj,
    const char *label,
    float conf,
    std::int64_t track_id) const
{
  NvOSD_TextParams *text = &obj->text_params;
  NvOSD_RectParams *rect = &obj->rect_params;
  if (!show_label_) {
    clear_label(obj);
  } else {
    char line[160];
    const char *name = label != nullptr ? label : "";
    std::snprintf(line, sizeof(line), "%s%c%.2f%c%lld", name, kLabelSep, conf, kLabelSep,
                  static_cast<long long>(track_id));
    g_free(text->display_text);
    text->display_text = g_strdup(line);
    text->x_offset = static_cast<unsigned int>(rect->left);
    int y = static_cast<int>(rect->top) - kLabelYOffset;
    text->y_offset = y < 0 ? 0u : static_cast<unsigned int>(y);
    text->font_params.font_name = const_cast<char *>(kFontName);
    text->font_params.font_size = kFontSize;
    text->set_bg_clr = 1;
  }
}

void DetFadeEngine::apply_style(
    NvDsObjectMeta *obj,
    const Rgba &box_color,
    const Rgba &text_color,
    const Rgba &text_bg_color) const
{
  set_rect_color(&obj->rect_params, box_color);
  write_label(obj);
  if (show_label_) {
    copy_color(&obj->text_params.font_params.font_color, text_color);
    copy_color(&obj->text_params.text_bg_clr, text_bg_color);
  }
}

void DetFadeEngine::decorate_object(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta,
    NvDsObjectMeta *obj,
    float fade_alpha)
{
  (void)batch_meta;
  (void)frame_meta;
  (void)obj;
  (void)fade_alpha;
}

void DetFadeEngine::copy_mask(
    CachedObject &cached,
    const float *data,
    unsigned int mask_size,
    unsigned int mask_width,
    unsigned int mask_height,
    float mask_threshold) const
{
  cached.mask.clear();
  cached.mask_size = 0;
  cached.mask_width = 0;
  cached.mask_height = 0;
  cached.mask_threshold = 0.0f;
  if (data != nullptr && mask_size > 0) {
    cached.mask_size = mask_size;
    cached.mask_width = mask_width;
    cached.mask_height = mask_height;
    cached.mask_threshold = mask_threshold;
    cached.mask.resize(mask_size / sizeof(float));
    std::memcpy(cached.mask.data(), data, mask_size);
  }
}

DetFadeEngine::CachedObject DetFadeEngine::cache_object(const NvDsObjectMeta *obj) const
{
  CachedObject cached;
  cached.left = obj->rect_params.left;
  cached.top = obj->rect_params.top;
  cached.width = obj->rect_params.width;
  cached.height = obj->rect_params.height;
  cached.confidence = obj->confidence;
  cached.class_id = obj->class_id;
  cached.object_id = obj->object_id;
  cached.label = obj->obj_label;
  copy_mask(
      cached,
      obj->mask_params.data,
      obj->mask_params.size,
      obj->mask_params.width,
      obj->mask_params.height,
      obj->mask_params.threshold);
  return cached;
}

DetFadeEngine::CachedObject DetFadeEngine::cache_object(const NvDsBboxSnapshotBox &box) const
{
  CachedObject cached;
  cached.left = box.left;
  cached.top = box.top;
  cached.width = box.width;
  cached.height = box.height;
  cached.confidence = box.confidence;
  cached.class_id = box.class_id;
  cached.object_id = box.object_id;
  cached.label = box.label;
  copy_mask(
      cached, box.mask, box.mask_size, box.mask_width, box.mask_height, box.mask_threshold);
  return cached;
}

void DetFadeEngine::restore_object(NvDsObjectMeta *obj, const CachedObject &cached) const
{
  obj->class_id = cached.class_id;
  obj->object_id = cached.object_id;
  obj->confidence = cached.confidence;
  obj->rect_params.left = cached.left;
  obj->rect_params.top = cached.top;
  obj->rect_params.width = cached.width;
  obj->rect_params.height = cached.height;
  g_strlcpy(obj->obj_label, cached.label.c_str(), sizeof(obj->obj_label));
  if (!cached.mask.empty() && cached.mask_size > 0) {
    obj->mask_params.width = cached.mask_width;
    obj->mask_params.height = cached.mask_height;
    obj->mask_params.size = cached.mask_size;
    obj->mask_params.threshold = cached.mask_threshold;
    obj->mask_params.data = static_cast<float *>(g_malloc(cached.mask_size));
    std::memcpy(obj->mask_params.data, cached.mask.data(), cached.mask_size);
  }
}

void DetFadeEngine::hide_tracker_mask(NvDsObjectMeta *obj) const
{
  (void)obj;
}

void DetFadeEngine::snapshot_objects(StreamState &state, const NvDsBboxSnapshotMeta *snapshot) const
{
  state.objects.clear();
  if (snapshot != nullptr && snapshot->boxes != nullptr) {
    for (guint i = 0; i < snapshot->num_boxes; i++) {
      state.objects.push_back(cache_object(snapshot->boxes[i]));
    }
  }
}

void DetFadeEngine::snapshot_frame_objects(StreamState &state, NvDsFrameMeta *frame_meta) const
{
  state.objects.clear();
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *obj = static_cast<NvDsObjectMeta *>(item->data);
    if (obj != nullptr) {
      state.objects.push_back(cache_object(obj));
    }
  }
}

void DetFadeEngine::style_frame_objects(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta,
    float fade_alpha,
    const Rgba &box_color,
    const Rgba &text_color,
    const Rgba &text_bg_color)
{
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *obj = static_cast<NvDsObjectMeta *>(item->data);
    if (obj != nullptr) {
      apply_style(obj, box_color, text_color, text_bg_color);
      decorate_object(batch_meta, frame_meta, obj, fade_alpha);
    }
  }
}

void DetFadeEngine::hide_tracker_visuals(NvDsFrameMeta *frame_meta)
{
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *obj = static_cast<NvDsObjectMeta *>(item->data);
    if (obj != nullptr) {
      obj->rect_params.border_width = 0;
      obj->rect_params.has_bg_color = 0;
      clear_label(obj);
      hide_tracker_mask(obj);
    }
  }
}

void DetFadeEngine::style_tracker_objects(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta)
{
  Rgba box_color = fade_color(kColorPurple, 1.0f);
  Rgba text_color = fade_color(kColorText, 1.0f);
  Rgba text_bg_color = fade_color(kColorTextBg, 1.0f);
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *obj = static_cast<NvDsObjectMeta *>(item->data);
    if (obj != nullptr) {
      apply_style(obj, box_color, text_color, text_bg_color);
      hide_tracker_mask(obj);
      decorate_object(batch_meta, frame_meta, obj, 1.0f);
    }
  }
}

void DetFadeEngine::inject_inferred_objects(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta,
    const StreamState &state,
    float fade_alpha,
    const Rgba &box_color,
    const Rgba &text_color,
    const Rgba &text_bg_color)
{
  for (const CachedObject &cached : state.objects) {
    NvDsObjectMeta *obj = nvds_acquire_obj_meta_from_pool(batch_meta);
    if (obj != nullptr) {
      restore_object(obj, cached);
      nvds_add_obj_meta_to_frame(frame_meta, obj, nullptr);
      apply_style(obj, box_color, text_color, text_bg_color);
      decorate_object(batch_meta, frame_meta, obj, fade_alpha);
    }
  }
}

void DetFadeEngine::process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  int pad_index = static_cast<int>(frame_meta->pad_index);
  StreamState &state = state_for(pad_index);
  bool inferred = frame_meta->bInferDone;
  if (inferred) {
    snapshot_frame_objects(state, frame_meta);
  }
  int lut_size = static_cast<int>(alpha_lut_.size());
  int phase = inferred ? 0 : state.phase;
  float fade_alpha = alpha_lut_[phase % lut_size];
  Rgba base = resolve_box_color(frame_meta);
  Rgba faded_box = fade_color(&base.r, fade_alpha);
  Rgba faded_text = fade_color(kColorText, fade_alpha);
  Rgba faded_text_bg = fade_color(kColorTextBg, fade_alpha);
  int next_phase = (phase + 1) % lut_size;
  if (inferred) {
    next_phase = lut_size > 0 ? 1 % lut_size : 0;
  }
  if (inferred) {
    style_frame_objects(
        batch_meta, frame_meta, fade_alpha, faded_box, faded_text, faded_text_bg);
  } else {
    inject_inferred_objects(
        batch_meta, frame_meta, state, fade_alpha, faded_box, faded_text, faded_text_bg);
  }
  state.phase = next_phase;
}

void DetFadeEngine::process_tracker_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  if (!show_snap_) {
    Rgba base = resolve_box_color(frame_meta);
    Rgba box_color = fade_color(&base.r, 1.0f);
    Rgba text_color = fade_color(kColorText, 1.0f);
    Rgba text_bg_color = fade_color(kColorTextBg, 1.0f);
    style_frame_objects(batch_meta, frame_meta, 1.0f, box_color, text_color, text_bg_color);
  } else {
    int pad_index = static_cast<int>(frame_meta->pad_index);
    StreamState &state = state_for(pad_index);
    const NvDsBboxSnapshotMeta *snapshot = find_snapshot_meta(frame_meta);
    bool inferred = frame_meta->bInferDone;
    if (inferred && snapshot != nullptr) {
      snapshot_objects(state, snapshot);
    }
    int lut_size = static_cast<int>(alpha_lut_.size());
    int phase = inferred ? 0 : state.phase;
    float fade_alpha = alpha_lut_[phase % lut_size];
    Rgba base = resolve_box_color(frame_meta);
    Rgba faded_box = fade_color(&base.r, fade_alpha);
    Rgba faded_text = fade_color(kColorText, fade_alpha);
    Rgba faded_text_bg = fade_color(kColorTextBg, fade_alpha);
    int next_phase = (phase + 1) % lut_size;
    if (inferred) {
      next_phase = lut_size > 0 ? 1 % lut_size : 0;
    }
    if (inferred) {
      hide_tracker_visuals(frame_meta);
    } else {
      style_tracker_objects(batch_meta, frame_meta);
    }
    inject_inferred_objects(
        batch_meta, frame_meta, state, fade_alpha, faded_box, faded_text, faded_text_bg);
    state.phase = next_phase;
  }
}

void DetFadeEngineWithTracker::process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  process_tracker_frame(batch_meta, frame_meta);
}

}  // namespace nvfadedrawer
