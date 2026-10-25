#include "logger/pipeline_logger.hpp"

#include "constants/service_log.hpp"

const char* PipelineLogger::name() const {
  return service_log::PIPELINE_DIR;
}
