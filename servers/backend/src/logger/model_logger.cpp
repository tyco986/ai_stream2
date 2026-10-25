#include "logger/model_logger.hpp"

#include "constants/service_log.hpp"

const char* ModelLogger::name() const {
  return service_log::MODEL_DIR;
}
