#include "presence_event_coder.hpp"

#include <cstdlib>
#include <cstring>

#include <glib.h>

namespace {

constexpr char kAbsent = '0';
constexpr char kAlert = '1';
constexpr char kTransit = '2';

void split_semi(const char *raw, std::vector<std::string> *parts)
{
  parts->clear();
  if (raw == nullptr || raw[0] == '\0') {
    return;
  }
  gchar **tokens = g_strsplit(raw, ";", -1);
  for (guint i = 0; tokens[i] != nullptr; i++) {
    g_strstrip(tokens[i]);
    if (tokens[i][0] != '\0') {
      parts->push_back(tokens[i]);
    }
  }
  g_strfreev(tokens);
}

}  // namespace

PresenceEventCoder::PresenceEventCoder()
{
  class_ids_str_ = "";
  event_names_str_ = "";
  mode_ = "fold";
  length_ = 10;
  threshold_ = 0.5f;
  is_slide_ = false;
}

void PresenceEventCoder::reset_pads()
{
  pads_.clear();
}

void PresenceEventCoder::set_class_ids(const char *class_ids)
{
  class_ids_str_ = class_ids != nullptr ? class_ids : "";
  class_id_list_.clear();
  std::vector<std::string> tokens;
  split_semi(class_ids_str_.c_str(), &tokens);
  for (const std::string &token : tokens) {
    class_id_list_.push_back(std::atoi(token.c_str()));
  }
  if (class_id_list_.size() > NVDS_PRESENCE_EVENT_CODE_LEN) {
    class_id_list_.resize(NVDS_PRESENCE_EVENT_CODE_LEN);
  }
  reset_pads();
}

void PresenceEventCoder::set_event_names(const char *event_names)
{
  event_names_str_ = event_names != nullptr ? event_names : "";
  name_list_.assign(NVDS_PRESENCE_EVENT_CODE_LEN, "");
  std::vector<std::string> tokens;
  split_semi(event_names_str_.c_str(), &tokens);
  guint n = tokens.size();
  if (n > NVDS_PRESENCE_EVENT_CODE_LEN) {
    n = NVDS_PRESENCE_EVENT_CODE_LEN;
  }
  for (guint i = 0; i < n; i++) {
    name_list_[i] = tokens[i];
  }
}

void PresenceEventCoder::set_length(int length)
{
  int next = length < 1 ? 1 : length;
  if (next != length_) {
    length_ = next;
    reset_pads();
  }
}

void PresenceEventCoder::set_threshold(float threshold)
{
  threshold_ = threshold;
}

void PresenceEventCoder::set_mode(const char *mode)
{
  std::string next = mode != nullptr ? mode : "fold";
  bool slide = next == "slide";
  bool fold = next == "fold";
  if (slide || fold) {
    mode_ = next;
    is_slide_ = slide;
    reset_pads();
  }
}

const std::string &PresenceEventCoder::class_ids() const
{
  return class_ids_str_;
}

const std::string &PresenceEventCoder::event_names() const
{
  return event_names_str_;
}

int PresenceEventCoder::length() const
{
  return length_;
}

float PresenceEventCoder::threshold() const
{
  return threshold_;
}

const std::string &PresenceEventCoder::mode() const
{
  return mode_;
}

PresenceEventCoder::PadState &PresenceEventCoder::state_for(int pad_index)
{
  PadState &pad = pads_[pad_index];
  if (pad.tracks.size() != class_id_list_.size()) {
    pad.tracks.clear();
    pad.tracks.resize(class_id_list_.size());
    for (ClassTrack &track : pad.tracks) {
      track.event_code = kAbsent;
      track.ratio = 0.0f;
    }
    pad.has_last = false;
    std::memset(&pad.last_meta, 0, sizeof(pad.last_meta));
  }
  return pad;
}

void PresenceEventCoder::update_tracks(
    PadState &pad, const std::unordered_set<int> &detected_ids)
{
  for (size_t i = 0; i < class_id_list_.size(); i++) {
    ClassTrack &track = pad.tracks[i];
    bool detected = detected_ids.count(class_id_list_[i]) > 0;
    track.detected_window.push_back(detected ? 1 : 0);
    if (static_cast<int>(track.detected_window.size()) >= length_) {
      int hits = 0;
      for (char flag : track.detected_window) {
        hits += flag;
      }
      track.ratio = static_cast<float>(hits) / static_cast<float>(length_);
      if (track.ratio >= threshold_) {
        track.event_code = detected ? kAlert : kTransit;
      } else {
        track.event_code = detected ? kTransit : kAbsent;
      }
      track.code_window.push_back(track.event_code);
      if (is_slide_) {
        track.detected_window.erase(track.detected_window.begin());
        if (!track.code_window.empty()) {
          track.code_window.erase(track.code_window.begin());
        }
      } else {
        track.detected_window.clear();
        track.code_window.clear();
      }
    } else {
      track.event_code = detected ? kTransit : kAbsent;
      track.code_window.push_back(track.event_code);
    }
  }
}

void PresenceEventCoder::write_meta(const PadState &pad, NvDsPresenceEventMeta *meta) const
{
  std::memset(meta, 0, sizeof(*meta));
  for (int i = 0; i < NVDS_PRESENCE_EVENT_CODE_LEN; i++) {
    meta->event_codes[i] = kAbsent;
  }
  guint n = static_cast<guint>(class_id_list_.size());
  meta->num_classes = n;
  for (guint i = 0; i < n; i++) {
    const ClassTrack &track = pad.tracks[i];
    meta->event_codes[i] = track.event_code;
    meta->classes[i].class_id = class_id_list_[i];
    meta->classes[i].ratio = track.ratio;
    meta->classes[i].counts[0] = 0;
    meta->classes[i].counts[1] = 0;
    meta->classes[i].counts[2] = 0;
    for (char code : track.code_window) {
      if (code >= kAbsent && code <= kTransit) {
        meta->classes[i].counts[code - kAbsent] += 1;
      }
    }
  }
  for (int i = 0; i < NVDS_PRESENCE_EVENT_CODE_LEN; i++) {
    if (meta->event_codes[i] == kAlert && i < static_cast<int>(name_list_.size())) {
      g_strlcpy(meta->event_names[i], name_list_[i].c_str(), NVDS_PRESENCE_EVENT_NAME_LEN);
    }
  }
}

void PresenceEventCoder::fill_meta(
    int pad_index,
    bool inferred,
    const std::unordered_set<int> &detected_ids,
    NvDsPresenceEventMeta *meta)
{
  PadState &pad = state_for(pad_index);
  if (inferred || !pad.has_last) {
    update_tracks(pad, detected_ids);
    write_meta(pad, meta);
    pad.last_meta = *meta;
    pad.has_last = true;
  } else {
    *meta = pad.last_meta;
  }
}
