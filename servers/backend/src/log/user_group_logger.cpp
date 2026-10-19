#include "log/user_group_logger.hpp"

#include "constants/log_const.hpp"

const char* UserGroupLogger::directory() const {
  return log::USER_GROUP_DIR;
}

const char* UserGroupLogger::name() const {
  return log::USER_GROUP_DIR;
}
