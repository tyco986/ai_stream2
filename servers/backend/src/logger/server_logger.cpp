#include "logger/server_logger.hpp"

#include "constants/service_log.hpp"

const char* ServerLogger::name() const {
  return service_log::SERVER_DIR;
}
