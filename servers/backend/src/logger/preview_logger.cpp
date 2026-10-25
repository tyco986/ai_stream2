#include "logger/preview_logger.hpp"

#include "constants/service_log.hpp"

const char* PreviewLogger::name() const {
  return service_log::PREVIEW_DIR;
}
