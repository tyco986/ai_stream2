#include "logger/proxy_logger.hpp"

#include "constants/service_log.hpp"

const char* ProxyLogger::name() const {
  return service_log::PROXY_DIR;
}
