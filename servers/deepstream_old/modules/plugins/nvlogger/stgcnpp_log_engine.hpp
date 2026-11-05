#pragma once

#include "det_log_engine.hpp"

namespace nvlogger {

class StgcnppLogEngine : public DetLogEngine {
 protected:
  const char *log_header() const override;
  void append_object_item(std::ostringstream &json, NvDsObjectMeta *object_meta) const override;
};

}  // namespace nvlogger
