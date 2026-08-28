#include "pose_fade_engine.hpp"

namespace nvfadedrawer {

void PoseFadeEngine::set_show_pose(bool show_pose)
{
  show_pose_ = show_pose;
}

void PoseFadeEngine::set_pose_threshold(float pose_threshold)
{
  pose_threshold_ = pose_threshold;
}

bool PoseFadeEngine::set_mode(const std::string &mode)
{
  bool ok = false;
  if (mode == "coco17") {
    pose_mode_ = PoseMode::Coco17;
    ok = true;
  } else if (mode == "openpose18") {
    ok = false;
  }
  return ok;
}

bool PoseFadeEngine::show_pose() const
{
  return show_pose_;
}

float PoseFadeEngine::pose_threshold() const
{
  return pose_threshold_;
}

const char *PoseFadeEngine::mode_name() const
{
  const char *name = "coco17";
  if (pose_mode_ == PoseMode::Coco17) {
    name = "coco17";
  }
  return name;
}

int PoseFadeEngine::clamp_x(float value, int frame_width) const
{
  int x = static_cast<int>(value + (value >= 0.0f ? 0.5f : -0.5f));
  if (x < 0) {
    x = 0;
  }
  if (x > frame_width - 1) {
    x = frame_width - 1;
  }
  return x;
}

int PoseFadeEngine::clamp_y(float value, int frame_height) const
{
  int y = static_cast<int>(value + (value >= 0.0f ? 0.5f : -0.5f));
  if (y < 0) {
    y = 0;
  }
  if (y > frame_height - 1) {
    y = frame_height - 1;
  }
  return y;
}

std::vector<float> PoseFadeEngine::decode_keypoints(NvDsObjectMeta *obj) const
{
  std::vector<float> keypoints;
  float *data = obj->mask_params.data;
  unsigned int width_dim = obj->mask_params.width;
  unsigned int height_dim = obj->mask_params.height;
  float bw = obj->rect_params.width;
  float bh = obj->rect_params.height;
  unsigned int k = height_dim;
  if (data == nullptr || bw <= 0.0f || bh <= 0.0f) {
    k = 0;
  } else if (width_dim == 3 && height_dim >= 1) {
    k = height_dim;
  } else if (width_dim >= 3 && height_dim == 0) {
    k = 0;
  } else if (width_dim > 0 && height_dim == 3) {
    k = width_dim;
  } else {
    k = 0;
  }
  if (k > 0) {
    keypoints.resize(static_cast<std::size_t>(k) * 3);
    for (unsigned int j = 0; j < k; j++) {
      keypoints[j * 3 + 0] = obj->rect_params.left + data[j * 3 + 0] * bw;
      keypoints[j * 3 + 1] = obj->rect_params.top + data[j * 3 + 1] * bh;
      keypoints[j * 3 + 2] = data[j * 3 + 2];
    }
  }
  return keypoints;
}

void PoseFadeEngine::draw_pose(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta,
    const std::vector<float> &keypoints,
    float fade_alpha)
{
  bool draw = show_pose_ && keypoints.size() >= 3;
  if (draw) {
    int frame_width = static_cast<int>(frame_meta->source_frame_width);
    int frame_height = static_cast<int>(frame_meta->source_frame_height);
    if (frame_width < 1) {
      frame_width = 1;
    }
    if (frame_height < 1) {
      frame_height = 1;
    }
    int kpt_n = static_cast<int>(keypoints.size() / 3);
    const int *edge_ptr = &kCoco17Edges[0][0];
    int n_edges = kCoco17EdgeCount;
    switch (pose_mode_) {
      case PoseMode::Coco17:
        edge_ptr = &kCoco17Edges[0][0];
        n_edges = kCoco17EdgeCount;
        break;
    }
    std::vector<NvOSD_CircleParams> circles;
    circles.reserve(static_cast<std::size_t>(kpt_n));
    for (int i = 0; i < kpt_n; i++) {
      float score = keypoints[static_cast<std::size_t>(i) * 3 + 2];
      const float *src = score < pose_threshold_ ? kColorRed : kColorOrange;
      Rgba faded = fade_color(src, fade_alpha);
      NvOSD_CircleParams circle{};
      circle.xc = clamp_x(keypoints[static_cast<std::size_t>(i) * 3 + 0], frame_width);
      circle.yc = clamp_y(keypoints[static_cast<std::size_t>(i) * 3 + 1], frame_height);
      circle.radius = kKptRadius;
      circle.circle_color.red = faded.r;
      circle.circle_color.green = faded.g;
      circle.circle_color.blue = faded.b;
      circle.circle_color.alpha = faded.a;
      circle.has_bg_color = 1;
      circle.bg_color = circle.circle_color;
      circles.push_back(circle);
    }
    std::vector<NvOSD_LineParams> lines;
    lines.reserve(static_cast<std::size_t>(n_edges));
    for (int e = 0; e < n_edges; e++) {
      int i = edge_ptr[e * 2 + 0];
      int j = edge_ptr[e * 2 + 1];
      if (i >= kpt_n || j >= kpt_n) {
        continue;
      }
      NvOSD_LineParams line{};
      line.x1 = clamp_x(keypoints[static_cast<std::size_t>(i) * 3 + 0], frame_width);
      line.y1 = clamp_y(keypoints[static_cast<std::size_t>(i) * 3 + 1], frame_height);
      line.x2 = clamp_x(keypoints[static_cast<std::size_t>(j) * 3 + 0], frame_width);
      line.y2 = clamp_y(keypoints[static_cast<std::size_t>(j) * 3 + 1], frame_height);
      Rgba faded_line = fade_color(kColorOrange, fade_alpha);
      line.line_width = kSkeletonWidth;
      line.line_color.red = faded_line.r;
      line.line_color.green = faded_line.g;
      line.line_color.blue = faded_line.b;
      line.line_color.alpha = faded_line.a;
      lines.push_back(line);
    }
    std::size_t c = 0;
    std::size_t l = 0;
    NvDsDisplayMeta *display = nullptr;
    while (c < circles.size() || l < lines.size()) {
      if (display == nullptr) {
        display = nvds_acquire_display_meta_from_pool(batch_meta);
        display->num_circles = 0;
        display->num_lines = 0;
      }
      bool added = false;
      if (c < circles.size() && display->num_circles < kMaxDisplayElements) {
        display->circle_params[display->num_circles] = circles[c];
        display->num_circles += 1;
        c += 1;
        added = true;
      }
      if (l < lines.size() && display->num_lines < kMaxDisplayElements) {
        display->line_params[display->num_lines] = lines[l];
        display->num_lines += 1;
        l += 1;
        added = true;
      }
      if (!added) {
        nvds_add_display_meta_to_frame(frame_meta, display);
        display = nullptr;
      }
    }
    if (display != nullptr) {
      nvds_add_display_meta_to_frame(frame_meta, display);
    }
  }
}

void PoseFadeEngine::decorate_object(
    NvDsBatchMeta *batch_meta,
    NvDsFrameMeta *frame_meta,
    NvDsObjectMeta *obj,
    float fade_alpha)
{
  draw_pose(batch_meta, frame_meta, decode_keypoints(obj), fade_alpha);
}

void PoseFadeEngineWithTracker::process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  process_tracker_frame(batch_meta, frame_meta);
}

}  // namespace nvfadedrawer
