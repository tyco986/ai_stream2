#pragma once

#include <string>
#include <vector>

#include "fade_core.hpp"

namespace nvfadedrawer {

enum class PoseMode {
  Coco17,
};

constexpr float kColorOrange[] = {1.0f, 0.5f, 0.0f, 1.0f};
constexpr int kKptRadius = 2;
constexpr int kSkeletonWidth = 2;
constexpr int kCoco17EdgeCount = 16;
constexpr int kCoco17Edges[kCoco17EdgeCount][2] = {
    {0, 1},  {0, 2},  {1, 3},  {2, 4},  {5, 6},  {5, 7},  {7, 9},  {6, 8},
    {8, 10}, {5, 11}, {6, 12}, {11, 12}, {11, 13}, {13, 15}, {12, 14}, {14, 16},
};

class PoseFadeEngine : public virtual DetFadeEngine {
 public:
  void set_show_pose(bool show_pose);
  void set_pose_threshold(float pose_threshold);
  bool set_mode(const std::string &mode);
  bool show_pose() const;
  float pose_threshold() const;
  const char *mode_name() const;

 protected:
  void decorate_object(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      NvDsObjectMeta *obj,
      float fade_alpha) override;
  void decorate_cached(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      const CachedObject &cached,
      float fade_alpha) override;

 private:
  std::vector<float> decode_keypoints(NvDsObjectMeta *obj) const;
  std::vector<float> decode_keypoints_from(
      const float *data,
      unsigned int width_dim,
      unsigned int height_dim,
      float left,
      float top,
      float bw,
      float bh) const;
  void draw_pose(
      NvDsBatchMeta *batch_meta,
      NvDsFrameMeta *frame_meta,
      const std::vector<float> &keypoints,
      float fade_alpha);
  int clamp_x(float value, int frame_width) const;
  int clamp_y(float value, int frame_height) const;

  bool show_pose_ = true;
  float pose_threshold_ = 0.0f;
  PoseMode pose_mode_ = PoseMode::Coco17;
};

class PoseFadeEngineWithTracker : public PoseFadeEngine, public DetFadeEngineWithTracker {};

}  // namespace nvfadedrawer
