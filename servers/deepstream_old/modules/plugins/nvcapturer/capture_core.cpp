#include "capture_core.hpp"

#include "gstnvcapturer_common.h"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <utility>

#include <cuda_runtime.h>
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
  stop_writer_ = false;
  parse_codes();
  writer_ = std::thread(&CaptureCore::writer_loop, this);
}

CaptureCore::~CaptureCore()
{
  stop_writer_ = true;
  writer_cv_.notify_all();
  if (writer_.joinable()) {
    writer_.join();
  }
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

const std::string &CaptureCore::label_task() const
{
  return label_task_;
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
  if (frame_meta != nullptr && frame_meta->bInferDone) {
    dump = codes_hit(meta);
  }
  return dump;
}

unsigned int CaptureCore::take_id(int pad_index)
{
  unsigned int id = ids_[pad_index];
  ids_[pad_index] = id + 1;
  return id;
}

bool CaptureCore::cuda_mem_type(NvBufSurfaceMemType mem_type) const
{
  bool cuda = mem_type == NVBUF_MEM_CUDA_DEVICE || mem_type == NVBUF_MEM_CUDA_PINNED ||
      mem_type == NVBUF_MEM_CUDA_UNIFIED || mem_type == NVBUF_MEM_DEFAULT;
  return cuda;
}

void CaptureCore::pack_rgb(
    const uint8_t *src,
    guint pitch,
    int width,
    int height,
    int channels,
    bool swap_rb,
    std::vector<uint8_t> *rgb)
{
  rgb->assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 3, 0);
  for (int y = 0; y < height; y++) {
    const uint8_t *row = src + static_cast<size_t>(y) * static_cast<size_t>(pitch);
    for (int x = 0; x < width; x++) {
      const uint8_t *px = row + static_cast<size_t>(x) * static_cast<size_t>(channels);
      size_t di = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 3;
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
}

bool CaptureCore::copy_rgb_cuda(
    NvBufSurfaceParams *params,
    int channels,
    bool swap_rb,
    std::vector<uint8_t> *rgb)
{
  bool ok = false;
  int width = static_cast<int>(params->width);
  int height = static_cast<int>(params->height);
  size_t row_bytes = static_cast<size_t>(width) * static_cast<size_t>(channels);
  if (params->dataPtr != nullptr && row_bytes > 0 &&
      row_bytes <= static_cast<size_t>(params->pitch)) {
    std::vector<uint8_t> packed(row_bytes * static_cast<size_t>(height));
    cudaError_t err = cudaMemcpy2D(
        packed.data(),
        row_bytes,
        params->dataPtr,
        static_cast<size_t>(params->pitch),
        row_bytes,
        static_cast<size_t>(height),
        cudaMemcpyDefault);
    if (err == cudaSuccess) {
      pack_rgb(packed.data(), static_cast<guint>(row_bytes), width, height, channels, swap_rb, rgb);
      ok = true;
    } else {
      g_warning("nvcapturer cudaMemcpy2D failed: %s", cudaGetErrorString(err));
    }
  }
  return ok;
}

bool CaptureCore::copy_rgb_mapped(
    NvBufSurface *surface,
    guint batch_id,
    NvBufSurfaceParams *params,
    int channels,
    bool swap_rb,
    std::vector<uint8_t> *rgb)
{
  bool ok = false;
  int batch = static_cast<int>(batch_id);
  if (NvBufSurfaceMap(surface, batch, -1, NVBUF_MAP_READ) == 0) {
    NvBufSurfaceSyncForCpu(surface, batch, -1);
    auto *src = static_cast<const uint8_t *>(params->mappedAddr.addr[0]);
    if (src != nullptr) {
      pack_rgb(src, params->pitch, static_cast<int>(params->width),
               static_cast<int>(params->height), channels, swap_rb, rgb);
      ok = true;
    }
    NvBufSurfaceUnMap(surface, batch, -1);
  }
  return ok;
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
    bool swap_rb = is_bgr_format(format);
    out_width = static_cast<int>(params->width);
    out_height = static_cast<int>(params->height);
    if (packed && out_width > 0 && out_height > 0) {
      if (cuda_mem_type(surface->memType)) {
        ok = copy_rgb_cuda(params, channels, swap_rb, rgb);
      }
      if (!ok) {
        ok = copy_rgb_mapped(surface, batch_id, params, channels, swap_rb, rgb);
      }
    }
  }
  *width = out_width;
  *height = out_height;
  return ok;
}

void CaptureCore::enqueue_dump(CaptureDumpJob job)
{
  {
    std::lock_guard<std::mutex> lock(writer_mutex_);
    writer_queue_.push_back(std::move(job));
  }
  writer_cv_.notify_one();
}

void CaptureCore::process_job(const CaptureDumpJob &job)
{
  bool png_ok = png_.write_rgb(job.png_path, job.rgb, job.width, job.height);
  bool labels_ok = true;
  if (job.write_labels) {
    labels_ok = write_det_labels(
        job.boxes, job.width, job.height, job.pad_index, job.capture_id);
  }
  if (!png_ok || !labels_ok) {
    g_warning("nvcapturer dump failed path=%s png=%d labels=%d", job.png_path.c_str(),
              static_cast<int>(png_ok), static_cast<int>(labels_ok));
  }
}

void CaptureCore::writer_loop()
{
  while (true) {
    CaptureDumpJob job;
    bool has_job = false;
    {
      std::unique_lock<std::mutex> lock(writer_mutex_);
      while (writer_queue_.empty() && !stop_writer_) {
        writer_cv_.wait(lock);
      }
      if (!writer_queue_.empty()) {
        job = std::move(writer_queue_.front());
        writer_queue_.pop_front();
        has_job = true;
      } else if (stop_writer_) {
        break;
      }
    }
    if (has_job) {
      process_job(job);
    }
  }
}

bool CaptureCore::dump_raw(NvBufSurface *surface, NvDsFrameMeta *frame_meta)
{
  bool ok = true;
  if (should_dump(frame_meta)) {
    int pad_index = static_cast<int>(frame_meta->pad_index);
    unsigned int capture_id = take_id(pad_index);
    char name[64];
    snprintf(name, sizeof(name), "raw_%03d_%08u.png", pad_index, capture_id);
    CaptureDumpJob job;
    job.png_path = output_dir_ + "/images/" + name;
    job.width = 0;
    job.height = 0;
    job.pad_index = pad_index;
    job.capture_id = capture_id;
    job.write_labels = false;
    ok = copy_rgb(surface, frame_meta->batch_id, &job.rgb, &job.width, &job.height);
    if (ok) {
      enqueue_dump(std::move(job));
    }
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
    CaptureDumpJob job;
    job.png_path = output_dir_ + "/vis/" + name;
    job.width = 0;
    job.height = 0;
    job.pad_index = pad_index;
    job.capture_id = capture_id;
    job.write_labels = true;
    collect_boxes(frame_meta, &job.boxes);
    ok = copy_rgb(surface, frame_meta->batch_id, &job.rgb, &job.width, &job.height);
    if (ok) {
      enqueue_dump(std::move(job));
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
