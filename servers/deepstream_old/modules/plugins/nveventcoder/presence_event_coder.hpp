#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nvds_presence_event_meta.h"

class PresenceEventCoder {
 public:
  PresenceEventCoder();

  void set_class_ids(const char *class_ids);
  void set_event_names(const char *event_names);
  void set_length(int length);
  void set_threshold(float threshold);
  void set_mode(const char *mode);

  const std::string &class_ids() const;
  const std::string &event_names() const;
  int length() const;
  float threshold() const;
  const std::string &mode() const;

  void fill_meta(
      int pad_index,
      bool inferred,
      const std::unordered_set<int> &detected_ids,
      NvDsPresenceEventMeta *meta);

 private:
  struct ClassTrack {
    std::vector<char> detected_window;
    std::vector<char> code_window;
    char event_code;
    float ratio;
  };

  struct PadState {
    std::vector<ClassTrack> tracks;
    NvDsPresenceEventMeta last_meta;
    bool has_last;
  };

  void reset_pads();
  PadState &state_for(int pad_index);
  void update_tracks(PadState &pad, const std::unordered_set<int> &detected_ids);
  void write_meta(const PadState &pad, NvDsPresenceEventMeta *meta) const;

  std::string class_ids_str_;
  std::string event_names_str_;
  std::string mode_;
  std::vector<int> class_id_list_;
  std::vector<std::string> name_list_;
  std::unordered_map<int, PadState> pads_;
  int length_;
  float threshold_;
  bool is_slide_;
};
