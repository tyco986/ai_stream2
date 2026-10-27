#pragma once

#include "logger/base_logger.hpp"

class ProxyLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
