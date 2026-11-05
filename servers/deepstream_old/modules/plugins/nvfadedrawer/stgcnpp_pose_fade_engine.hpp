#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "pose_fade_engine.hpp"

namespace nvfadedrawer {

class StgcnppPoseFadeEngine : public PoseFadeEngineWithTracker {
 public:
  void process_frame(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta) override;

 protected:
  void write_label(NvDsObjectMeta *obj) const override;

 private:
  struct LastAction {
    std::string name;
    float conf = 0.0f;
  };

  void cache_live_actions(NvDsFrameMeta *frame_meta);

  mutable std::unordered_map<std::uint64_t, LastAction> last_actions_;
};

}  // namespace nvfadedrawer
