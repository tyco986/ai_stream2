#pragma once

#include "logger/base_logger.hpp"

class LogLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
