#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nvbufsurface.h"
#include "nvds_presence_event_meta.h"
#include "nvdsmeta.h"
#include "png_writer.hpp"

struct CaptureBox {
  float left;
  float top;
  float width;
  float height;
  int class_id;
  std::string label;
};

class CaptureCore {
 public:
  CaptureCore();

  void set_output_dir(const char *output_dir);
  void set_capture_codes(const char *capture_codes);
  void set_interval(int interval);
  void set_label_task(const char *label_task);

  const std::string &output_dir() const;
  const std::string &capture_codes() const;
  int interval() const;
  const std::string &label_task() const;

  bool should_dump(NvDsFrameMeta *frame_meta) const;
  unsigned int take_id(int pad_index);
  bool dump_raw(NvBufSurface *surface, NvDsFrameMeta *frame_meta);
  bool dump_vis(NvBufSurface *surface, NvDsFrameMeta *frame_meta);
  bool write_png(NvBufSurface *surface, guint batch_id, const std::string &path);
  void collect_boxes(NvDsFrameMeta *frame_meta, std::vector<CaptureBox> *boxes) const;
  bool write_det_labels(
      const std::vector<CaptureBox> &boxes,
      int image_width,
      int image_height,
      int pad_index,
      unsigned int capture_id);

 private:
  void parse_codes();
  bool is_inference_frame(unsigned int frame_num) const;
  const NvDsPresenceEventMeta *presence_meta(NvDsFrameMeta *frame_meta) const;
  bool codes_hit(const NvDsPresenceEventMeta *meta) const;
  bool copy_rgb(
      NvBufSurface *surface,
      guint batch_id,
      std::vector<uint8_t> *rgb,
      int *width,
      int *height);
  std::string json_escape(const std::string &text) const;
  bool write_yolo(
      const std::string &path,
      const std::vector<CaptureBox> &boxes,
      int image_width,
      int image_height);
  bool write_labelme(
      const std::string &path,
      const std::vector<CaptureBox> &boxes,
      int image_width,
      int image_height,
      const std::string &image_path);

  std::string output_dir_;
  std::string capture_codes_str_;
  std::string label_task_;
  std::unordered_set<char> codes_;
  std::unordered_map<int, unsigned int> ids_;
  PngWriter png_;
  int interval_;
};
