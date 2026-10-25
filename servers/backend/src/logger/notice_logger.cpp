#include "logger/notice_logger.hpp"

#include "constants/service_log.hpp"

const char* NoticeLogger::name() const {
  return service_log::NOTICE_DIR;
}
