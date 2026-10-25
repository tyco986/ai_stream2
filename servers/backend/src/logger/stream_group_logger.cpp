#include "logger/stream_group_logger.hpp"

#include "constants/service_log.hpp"

const char* StreamGroupLogger::name() const {
  return service_log::STREAM_GROUP_DIR;
}
