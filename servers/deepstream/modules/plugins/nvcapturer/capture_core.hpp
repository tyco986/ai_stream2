#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
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

struct CaptureDumpJob {
  std::string png_path;
  std::vector<uint8_t> rgb;
  std::vector<CaptureBox> boxes;
  int width;
  int height;
  int pad_index;
  unsigned int capture_id;
  bool write_labels;
};

class CaptureCore {
 public:
  CaptureCore();
  ~CaptureCore();
  CaptureCore(const CaptureCore &) = delete;
  CaptureCore &operator=(const CaptureCore &) = delete;

  void set_output_dir(const char *output_dir);
  void set_capture_codes(const char *capture_codes);
  void set_label_task(const char *label_task);

  const std::string &output_dir() const;
  const std::string &capture_codes() const;
  const std::string &label_task() const;

  bool should_dump(NvDsFrameMeta *frame_meta) const;
  unsigned int take_id(int pad_index);
  bool dump_raw(NvBufSurface *surface, NvDsFrameMeta *frame_meta);
  bool dump_vis(NvBufSurface *surface, NvDsFrameMeta *frame_meta);
  void collect_boxes(NvDsFrameMeta *frame_meta, std::vector<CaptureBox> *boxes) const;
  bool write_det_labels(
      const std::vector<CaptureBox> &boxes,
      int image_width,
      int image_height,
      int pad_index,
      unsigned int capture_id);

 private:
  void parse_codes();
  const NvDsPresenceEventMeta *presence_meta(NvDsFrameMeta *frame_meta) const;
  bool codes_hit(const NvDsPresenceEventMeta *meta) const;
  bool copy_rgb(
      NvBufSurface *surface,
      guint batch_id,
      std::vector<uint8_t> *rgb,
      int *width,
      int *height);
  bool copy_rgb_cuda(
      NvBufSurfaceParams *params,
      int channels,
      bool swap_rb,
      std::vector<uint8_t> *rgb);
  bool copy_rgb_mapped(
      NvBufSurface *surface,
      guint batch_id,
      NvBufSurfaceParams *params,
      int channels,
      bool swap_rb,
      std::vector<uint8_t> *rgb);
  void pack_rgb(
      const uint8_t *src,
      guint pitch,
      int width,
      int height,
      int channels,
      bool swap_rb,
      std::vector<uint8_t> *rgb);
  bool cuda_mem_type(NvBufSurfaceMemType mem_type) const;
  void enqueue_dump(CaptureDumpJob job);
  void writer_loop();
  void process_job(const CaptureDumpJob &job);
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
  std::atomic<bool> stop_writer_;
  std::mutex writer_mutex_;
  std::condition_variable writer_cv_;
  std::deque<CaptureDumpJob> writer_queue_;
  std::thread writer_;
};
