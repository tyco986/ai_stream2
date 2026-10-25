#pragma once

#include "logger/base_logger.hpp"

class MediaLogger : public BaseLogger {
 protected:
  const char* name() const override;
};
