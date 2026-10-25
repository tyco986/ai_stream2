#pragma once

#include "logger/base_logger.hpp"

class StreamGroupLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
