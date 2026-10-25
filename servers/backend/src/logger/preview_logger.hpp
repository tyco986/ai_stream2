#pragma once

#include "logger/base_logger.hpp"

class PreviewLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
