#pragma once

#include "logger/base_logger.hpp"

class NoticeLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
