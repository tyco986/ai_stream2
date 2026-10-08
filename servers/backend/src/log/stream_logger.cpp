#include "log/stream_logger.hpp"

#include "constants/log.hpp"

const char* StreamLogger::directory() const {
  return log::STREAM_DIR;
}

const char* StreamLogger::name() const {
  return log::STREAM_DIR;
}
