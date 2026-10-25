#pragma once

#include "logger/base_logger.hpp"

class PipelineAnalyzerLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
