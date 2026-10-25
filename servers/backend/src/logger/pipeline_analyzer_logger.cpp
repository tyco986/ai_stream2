#include "logger/pipeline_analyzer_logger.hpp"

#include "constants/service_log.hpp"

const char* PipelineAnalyzerLogger::name() const {
  return service_log::PIPELINE_ANALYZER_DIR;
}
