#pragma once

#include "log/base_logger.hpp"

class PipelineAnalyzerLogger : public BaseLogger {
 protected:
  const char* directory() const override;
  const char* name() const override;
};
