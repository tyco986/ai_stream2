#include "log/stream_logger.hpp"

#include "constants/log_const.hpp"

const char* StreamLogger::directory() const {
  return log::STREAM_DIR;
}

const char* StreamLogger::name() const {
  return log::STREAM_DIR;
}
