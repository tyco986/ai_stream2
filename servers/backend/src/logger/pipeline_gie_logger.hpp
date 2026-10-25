#pragma once

#include "logger/base_logger.hpp"

class PipelineGieLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
