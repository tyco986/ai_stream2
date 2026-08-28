#include "capture_core.hpp"

#include "gstnvcapturer_common.h"

#include <cstdio>
#include <cstring>
#include <sstream>

#include <glib.h>

namespace {

void split_codes(const std::string &raw, std::unordered_set<char> *codes)
{
  codes->clear();
  for (char item : raw) {
    if (item != ';' && item != ' ' && item != '\t') {
      codes->insert(item);
    }
  }
}

bool is_rgb_format(NvBufSurfaceColorFormat format)
{
  bool rgb = format == NVBUF_COLOR_FORMAT_RGB || format == NVBUF_COLOR_FORMAT_RGBA ||
      format == NVBUF_COLOR_FORMAT_RGBx;
  return rgb;
}

bool is_bgr_format(NvBufSurfaceColorFormat format)
{
  bool bgr = format == NVBUF_COLOR_FORMAT_BGR || format == NVBUF_COLOR_FORMAT_BGRA ||
      format == NVBUF_COLOR_FORMAT_BGRx;
  return bgr;
}

int channel_count(NvBufSurfaceColorFormat format)
{
  int count = 3;
  if (format == NVBUF_COLOR_FORMAT_RGBA || format == NVBUF_COLOR_FORMAT_BGRA ||
      format == NVBUF_COLOR_FORMAT_RGBx || format == NVBUF_COLOR_FORMAT_BGRx) {
    count = 4;
  }
  return count;
}

}  // namespace

CaptureCore::CaptureCore()
{
  output_dir_ = NVCAPTURER_DEFAULT_OUTPUT_DIR;
  capture_codes_str_ = NVCAPTURER_DEFAULT_CAPTURE_CODES;
  label_task_ = NVCAPTURER_DEFAULT_LABEL_TASK;
  interval_ = 0;
  parse_codes();
}

void CaptureCore::parse_codes()
{
  split_codes(capture_codes_str_, &codes_);
  if (codes_.empty()) {
    codes_.insert('1');
  }
}

void CaptureCore::set_output_dir(const char *output_dir)
{
  output_dir_ = (output_dir != nullptr && output_dir[0] != '\0')
      ? output_dir
      : NVCAPTURER_DEFAULT_OUTPUT_DIR;
  ids_.clear();
}

void CaptureCore::set_capture_codes(const char *capture_codes)
{
  capture_codes_str_ = (capture_codes != nullptr && capture_codes[0] != '\0')
      ? capture_codes
      : NVCAPTURER_DEFAULT_CAPTURE_CODES;
  parse_codes();
}

void CaptureCore::set_interval(int interval)
{
  interval_ = interval < 0 ? 0 : interval;
}

void CaptureCore::set_label_task(const char *label_task)
{
  std::string next = (label_task != nullptr && label_task[0] != '\0')
      ? label_task
      : NVCAPTURER_DEFAULT_LABEL_TASK;
  if (next == "det" || next == "seg") {
    label_task_ = next;
  }
}

const std::string &CaptureCore::output_dir() const
{
  return output_dir_;
}

const std::string &CaptureCore::capture_codes() const
{
  return capture_codes_str_;
}

int CaptureCore::interval() const
{
  return interval_;
}

const std::string &CaptureCore::label_task() const
{
  return label_task_;
}

bool CaptureCore::is_inference_frame(unsigned int frame_num) const
{
  bool inference = interval_ <= 0 || (static_cast<int>(frame_num) % interval_) == 0;
  return inference;
}

const NvDsPresenceEventMeta *CaptureCore::presence_meta(NvDsFrameMeta *frame_meta) const
{
  const NvDsPresenceEventMeta *meta = nullptr;
  if (frame_meta != nullptr) {
    for (NvDsMetaList *item = frame_meta->frame_user_meta_list; item != nullptr;
         item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta != nullptr &&
          user_meta->base_meta.meta_type == NVDS_PRESENCE_EVENT_USER_META) {
        meta = static_cast<NvDsPresenceEventMeta *>(user_meta->user_meta_data);
      }
    }
  }
  return meta;
}

bool CaptureCore::codes_hit(const NvDsPresenceEventMeta *meta) const
{
  bool hit = false;
  if (meta != nullptr) {
    guint n = meta->num_classes;
    if (n > NVDS_PRESENCE_EVENT_CODE_LEN) {
      n = NVDS_PRESENCE_EVENT_CODE_LEN;
    }
    for (guint i = 0; i < n; i++) {
      if (codes_.count(meta->event_codes[i]) > 0) {
        hit = true;
      }
    }
  }
  return hit;
}

bool CaptureCore::should_dump(NvDsFrameMeta *frame_meta) const
{
  const NvDsPresenceEventMeta *meta = presence_meta(frame_meta);
  bool dump = false;
  if (frame_meta != nullptr && meta != nullptr) {
    dump = is_inference_frame(frame_meta->frame_num) && codes_hit(meta);
  }
  return dump;
}

unsigned int CaptureCore::take_id(int pad_index)
{
  unsigned int id = ids_[pad_index];
  ids_[pad_index] = id + 1;
  return id;
}

bool CaptureCore::copy_rgb(
    NvBufSurface *surface,
    guint batch_id,
    std::vector<uint8_t> *rgb,
    int *width,
    int *height)
{
  bool ok = false;
  int out_width = 0;
  int out_height = 0;
  rgb->clear();
  if (surface != nullptr && batch_id < surface->numFilled) {
    NvBufSurfaceParams *params = &surface->surfaceList[batch_id];
    NvBufSurfaceColorFormat format = params->colorFormat;
    bool packed = is_rgb_format(format) || is_bgr_format(format);
    int channels = channel_count(format);
    out_width = static_cast<int>(params->width);
    out_height = static_cast<int>(params->height);
    if (packed && out_width > 0 && out_height > 0 &&
        NvBufSurfaceMap(surface, static_cast<int>(batch_id), -1, NVBUF_MAP_READ) == 0) {
      NvBufSurfaceSyncForCpu(surface, static_cast<int>(batch_id), -1);
      auto *src = static_cast<const uint8_t *>(params->mappedAddr.addr[0]);
      if (src != nullptr) {
        bool swap_rb = is_bgr_format(format);
        guint pitch = params->pitch;
        rgb->assign(static_cast<size_t>(out_width) * static_cast<size_t>(out_height) * 3, 0);
        for (int y = 0; y < out_height; y++) {
          const uint8_t *row = src + static_cast<size_t>(y) * static_cast<size_t>(pitch);
          for (int x = 0; x < out_width; x++) {
            const uint8_t *px = row + static_cast<size_t>(x) * static_cast<size_t>(channels);
            size_t di =
                (static_cast<size_t>(y) * static_cast<size_t>(out_width) + static_cast<size_t>(x)) *
                3;
            uint8_t r = px[0];
            uint8_t g = px[1];
            uint8_t b = px[2];
            if (swap_rb) {
              r = px[2];
              b = px[0];
            }
            (*rgb)[di] = r;
            (*rgb)[di + 1] = g;
            (*rgb)[di + 2] = b;
          }
        }
        ok = true;
      }
      NvBufSurfaceUnMap(surface, static_cast<int>(batch_id), -1);
    }
  }
  *width = out_width;
  *height = out_height;
  return ok;
}

bool CaptureCore::write_png(NvBufSurface *surface, guint batch_id, const std::string &path)
{
  std::vector<uint8_t> rgb;
  int width = 0;
  int height = 0;
  bool ok = false;
  if (copy_rgb(surface, batch_id, &rgb, &width, &height)) {
    ok = png_.write_rgb(path, rgb, width, height);
  }
  return ok;
}

bool CaptureCore::dump_raw(NvBufSurface *surface, NvDsFrameMeta *frame_meta)
{
  bool ok = true;
  if (should_dump(frame_meta)) {
    int pad_index = static_cast<int>(frame_meta->pad_index);
    unsigned int capture_id = take_id(pad_index);
    char name[64];
    snprintf(name, sizeof(name), "raw_%03d_%08u.png", pad_index, capture_id);
    std::string path = output_dir_ + "/images/" + name;
    ok = write_png(surface, frame_meta->batch_id, path);
  }
  return ok;
}

bool CaptureCore::dump_vis(NvBufSurface *surface, NvDsFrameMeta *frame_meta)
{
  bool ok = true;
  if (should_dump(frame_meta)) {
    int pad_index = static_cast<int>(frame_meta->pad_index);
    unsigned int capture_id = take_id(pad_index);
    char name[64];
    snprintf(name, sizeof(name), "vis_%03d_%08u.png", pad_index, capture_id);
    std::string path = output_dir_ + "/vis/" + name;
    std::vector<uint8_t> rgb;
    int width = 0;
    int height = 0;
    std::vector<CaptureBox> boxes;
    collect_boxes(frame_meta, &boxes);
    ok = copy_rgb(surface, frame_meta->batch_id, &rgb, &width, &height);
    if (ok) {
      ok = png_.write_rgb(path, rgb, width, height);
    }
    if (ok) {
      ok = write_det_labels(boxes, width, height, pad_index, capture_id);
    }
  }
  return ok;
}

void CaptureCore::collect_boxes(NvDsFrameMeta *frame_meta, std::vector<CaptureBox> *boxes) const
{
  boxes->clear();
  NvDsMetaList *item = frame_meta != nullptr ? frame_meta->obj_meta_list : nullptr;
  for (; item != nullptr; item = item->next) {
    auto *object_meta = static_cast<NvDsObjectMeta *>(item->data);
    if (object_meta != nullptr) {
      CaptureBox box;
      box.left = object_meta->rect_params.left;
      box.top = object_meta->rect_params.top;
      box.width = object_meta->rect_params.width;
      box.height = object_meta->rect_params.height;
      box.class_id = object_meta->class_id;
      if (object_meta->obj_label[0] != '\0') {
        box.label = object_meta->obj_label;
      } else {
        box.label = std::to_string(object_meta->class_id);
      }
      boxes->push_back(box);
    }
  }
}

std::string CaptureCore::json_escape(const std::string &text) const
{
  std::string escaped;
  escaped.reserve(text.size());
  for (char item : text) {
    if (item == '\\' || item == '"') {
      escaped.push_back('\\');
    }
    escaped.push_back(item);
  }
  return escaped;
}

bool CaptureCore::write_yolo(
    const std::string &path,
    const std::vector<CaptureBox> &boxes,
    int image_width,
    int image_height)
{
  bool ok = false;
  FILE *file = nullptr;
  gchar *parent = g_path_get_dirname(path.c_str());
  g_mkdir_with_parents(parent, 0755);
  g_free(parent);
  file = fopen(path.c_str(), "wb");
  if (file != nullptr) {
    ok = true;
    if (image_width > 0 && image_height > 0) {
      for (const CaptureBox &box : boxes) {
        float cx = (box.left + box.width / 2.0f) / static_cast<float>(image_width);
        float cy = (box.top + box.height / 2.0f) / static_cast<float>(image_height);
        float bw = box.width / static_cast<float>(image_width);
        float bh = box.height / static_cast<float>(image_height);
        if (fprintf(file, "%d %.6f %.6f %.6f %.6f\n", box.class_id, cx, cy, bw, bh) < 0) {
          ok = false;
        }
      }
    }
    if (fclose(file) != 0) {
      ok = false;
    }
  }
  return ok;
}

bool CaptureCore::write_labelme(
    const std::string &path,
    const std::vector<CaptureBox> &boxes,
    int image_width,
    int image_height,
    const std::string &image_path)
{
  bool ok = false;
  FILE *file = nullptr;
  gchar *parent = g_path_get_dirname(path.c_str());
  g_mkdir_with_parents(parent, 0755);
  g_free(parent);
  file = fopen(path.c_str(), "wb");
  if (file != nullptr) {
    std::ostringstream body;
    body << "{\n";
    body << "  \"version\": \"5.0.1\",\n";
    body << "  \"flags\": {},\n";
    body << "  \"shapes\": [\n";
    for (size_t i = 0; i < boxes.size(); i++) {
      const CaptureBox &box = boxes[i];
      float right = box.left + box.width;
      float bottom = box.top + box.height;
      body << "    {\n";
      body << "      \"label\": \"" << json_escape(box.label) << "\",\n";
      body << "      \"points\": [[" << box.left << ", " << box.top << "], [" << right
           << ", " << bottom << "]],\n";
      body << "      \"group_id\": null,\n";
      body << "      \"shape_type\": \"rectangle\",\n";
      body << "      \"flags\": {}\n";
      body << "    }";
      if (i + 1 < boxes.size()) {
        body << ",";
      }
      body << "\n";
    }
    body << "  ],\n";
    body << "  \"imagePath\": \"" << json_escape(image_path) << "\",\n";
    body << "  \"imageData\": null,\n";
    body << "  \"imageHeight\": " << image_height << ",\n";
    body << "  \"imageWidth\": " << image_width << "\n";
    body << "}\n";
    std::string text = body.str();
    ok = fwrite(text.data(), 1, text.size(), file) == text.size();
    if (fclose(file) != 0) {
      ok = false;
    }
  }
  return ok;
}

bool CaptureCore::write_det_labels(
    const std::vector<CaptureBox> &boxes,
    int image_width,
    int image_height,
    int pad_index,
    unsigned int capture_id)
{
  char stem[64];
  snprintf(stem, sizeof(stem), "raw_%03d_%08u", pad_index, capture_id);
  std::string yolo_path = output_dir_ + "/labels/" + stem + ".txt";
  std::string json_path = output_dir_ + "/labelme/" + stem + ".json";
  std::string image_path = std::string("../images/") + stem + ".png";
  bool yolo_ok = write_yolo(yolo_path, boxes, image_width, image_height);
  bool json_ok = write_labelme(json_path, boxes, image_width, image_height, image_path);
  bool ok = yolo_ok && json_ok;
  return ok;
}
