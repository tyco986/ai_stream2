#include "logger/media_logger.hpp"

#include "constants/service_log.hpp"

const char* MediaLogger::name() const {
  return service_log::MEDIA_DIR;
}
