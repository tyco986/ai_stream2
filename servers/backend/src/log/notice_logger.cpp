#include "log/notice_logger.hpp"

#include "constants/log_const.hpp"

const char* NoticeLogger::directory() const {
  return log::NOTICE_DIR;
}

const char* NoticeLogger::name() const {
  return log::NOTICE_DIR;
}
