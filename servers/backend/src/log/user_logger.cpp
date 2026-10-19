#include "log/user_logger.hpp"

#include "constants/log_const.hpp"

const char* UserLogger::directory() const {
  return log::USER_DIR;
}

const char* UserLogger::name() const {
  return log::USER_DIR;
}
