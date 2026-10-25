#pragma once

#include "logger/base_logger.hpp"

class ModelLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
