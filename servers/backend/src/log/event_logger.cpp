#include "log/event_logger.hpp"

#include "constants/log_const.hpp"

const char* EventLogger::directory() const {
  return log::EVENT_DIR;
}

const char* EventLogger::name() const {
  return log::EVENT_DIR;
}
