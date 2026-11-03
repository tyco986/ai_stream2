#include "logger/device_logger.hpp"

#include "constants/service_log.hpp"

const char* DeviceLogger::name() const {
  return service_log::DEVICE_DIR;
}
