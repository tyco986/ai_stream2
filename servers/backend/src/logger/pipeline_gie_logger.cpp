#include "logger/pipeline_gie_logger.hpp"

#include "constants/service_log.hpp"

const char* PipelineGieLogger::name() const {
  return service_log::PIPELINE_GIE_DIR;
}
