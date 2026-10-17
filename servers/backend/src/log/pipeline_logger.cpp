#include "log/pipeline_logger.hpp"

#include "constants/log_const.hpp"

const char* PipelineLogger::directory() const {
  return log::PIPELINE_DIR;
}

const char* PipelineLogger::name() const {
  return log::PIPELINE_DIR;
}
