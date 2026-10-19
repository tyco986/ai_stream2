#include "log/server_logger.hpp"

#include "constants/log_const.hpp"

const char* ServerLogger::directory() const {
  return log::SERVER_DIR;
}

const char* ServerLogger::name() const {
  return log::SERVER_DIR;
}
