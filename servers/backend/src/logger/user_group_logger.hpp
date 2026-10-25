#pragma once

#include "logger/base_logger.hpp"

class UserGroupLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
