#include "logger/user_logger.hpp"

#include "constants/service_log.hpp"

const char* UserLogger::name() const {
  return service_log::USER_DIR;
}
