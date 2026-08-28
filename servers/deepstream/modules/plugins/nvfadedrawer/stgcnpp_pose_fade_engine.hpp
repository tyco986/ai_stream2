#pragma once

#include "pose_fade_engine.hpp"

namespace nvfadedrawer {

class StgcnppPoseFadeEngine : public PoseFadeEngineWithTracker {
 public:
  StgcnppPoseFadeEngine();
  void set_classifier_unique_id(int classifier_unique_id);
  int classifier_unique_id() const;

 protected:
  void write_label(NvDsObjectMeta *obj) const override;

 private:
  void read_action(NvDsObjectMeta *obj, const char **action_name, float *action_conf) const;

  int classifier_unique_id_;
};

}  // namespace nvfadedrawer
