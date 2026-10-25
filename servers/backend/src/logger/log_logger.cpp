#include "logger/log_logger.hpp"

#include "constants/service_log.hpp"

const char* LogLogger::name() const {
  return service_log::LOG_DIR;
}
