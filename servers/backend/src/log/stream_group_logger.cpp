#include "log/stream_group_logger.hpp"

#include "constants/log.hpp"

const char* StreamGroupLogger::directory() const {
  return log::STREAM_GROUP_DIR;
}

const char* StreamGroupLogger::name() const {
  return log::STREAM_GROUP_DIR;
}
