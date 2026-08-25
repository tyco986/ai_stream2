#pragma once

#include <string>
#include <vector>

#include "fade_core.hpp"

namespace nvfadedrawer {

enum class PoseMode {
  Coco17,
};

class PoseFadeEngine : public DetFadeEngine {
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

 private:
  std::vector<float> decode_keypoints(NvDsObjectMeta *obj) const;
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

}  // namespace nvfadedrawer
