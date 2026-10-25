#pragma once

#include "logger/base_logger.hpp"

class StreamLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
