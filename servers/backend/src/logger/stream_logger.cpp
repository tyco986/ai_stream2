#include "logger/stream_logger.hpp"

#include "constants/service_log.hpp"

const char* StreamLogger::name() const {
  return service_log::STREAM_DIR;
}
