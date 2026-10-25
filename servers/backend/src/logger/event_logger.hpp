#pragma once

#include "logger/base_logger.hpp"

class EventLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
