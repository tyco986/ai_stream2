#include "log/preview_logger.hpp"

#include "constants/log_const.hpp"

const char* PreviewLogger::directory() const {
  return log::PREVIEW_DIR;
}

const char* PreviewLogger::name() const {
  return log::PREVIEW_DIR;
}
