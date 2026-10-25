#include "logger/user_group_logger.hpp"

#include "constants/service_log.hpp"

const char* UserGroupLogger::name() const {
  return service_log::USER_GROUP_DIR;
}
