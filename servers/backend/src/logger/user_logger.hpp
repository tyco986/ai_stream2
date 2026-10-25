#pragma once

#include "logger/base_logger.hpp"

class UserLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
