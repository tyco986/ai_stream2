#include "fade_core.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>

#include <glib.h>

#include "nvds_fade_event_meta.h"

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

const NvDsFadeEventMeta *find_event_meta(NvDsFrameMeta *frame_meta)
{
  const NvDsFadeEventMeta *event = nullptr;
  for (NvDsMetaList *item = frame_meta->frame_user_meta_list; item != nullptr; item = item->next) {
    auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
    if (user_meta == nullptr || user_meta->base_meta.meta_type != NVDS_FADE_EVENT_USER_META) {
      continue;
    }
    event = static_cast<const NvDsFadeEventMeta *>(user_meta->user_meta_data);
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

void DetFadeEngine::drop_pad(int pad_index)
{
  streams_.erase(pad_index);
}

bool DetFadeEngine::is_inference_frame(unsigned int frame_num) const
{
  bool inference = interval_ <= 0 || (static_cast<int>(frame_num) % interval_) == 0;
  return inference;
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
  const NvDsFadeEventMeta *event = find_event_meta(frame_meta);
  if (event != nullptr) {
    bool has_alert = false;
    bool has_transit = false;
    for (int i = 0; i < kEventCodeLen; i++) {
      char code = event->event_codes[i];
      if (code == '\0') {
        break;
      }
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
  fill_label(obj, obj->obj_label, obj->confidence, display_id(obj->object_id));
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

void DetFadeEngine::process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  int pad_index = static_cast<int>(frame_meta->pad_index);
  StreamState &state = state_for(pad_index);
  bool inference = is_inference_frame(frame_meta->frame_num);
  int lut_size = static_cast<int>(alpha_lut_.size());
  int phase = inference ? 0 : state.phase;
  float fade_alpha = alpha_lut_[phase % lut_size];
  Rgba base = resolve_box_color(frame_meta);
  Rgba faded_box = fade_color(&base.r, fade_alpha);
  Rgba faded_text = fade_color(kColorText, fade_alpha);
  Rgba faded_text_bg = fade_color(kColorTextBg, fade_alpha);
  int next_phase = (phase + 1) % lut_size;
  if (inference) {
    next_phase = lut_size > 0 ? 1 % lut_size : 0;
  }
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *obj = static_cast<NvDsObjectMeta *>(item->data);
    if (obj == nullptr) {
      continue;
    }
    apply_style(obj, faded_box, faded_text, faded_text_bg);
    decorate_object(batch_meta, frame_meta, obj, fade_alpha);
  }
  state.phase = next_phase;
}

}  // namespace nvfadedrawer
