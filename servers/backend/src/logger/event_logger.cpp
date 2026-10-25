#include "logger/event_logger.hpp"

#include "constants/service_log.hpp"

const char* EventLogger::name() const {
  return service_log::EVENT_DIR;
}
