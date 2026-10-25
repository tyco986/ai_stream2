#pragma once

#include "logger/base_logger.hpp"

class PipelineLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
