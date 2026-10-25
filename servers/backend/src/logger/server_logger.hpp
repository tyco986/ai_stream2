#pragma once

#include "logger/base_logger.hpp"

class ServerLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
