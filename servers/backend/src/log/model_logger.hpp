#pragma once

#include "log/base_logger.hpp"

class ModelLogger : public BaseLogger {
 protected:
  const char* directory() const override;
  const char* name() const override;
};
