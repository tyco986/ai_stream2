#include "error_tracer/sql.hpp"

std::string Sql::locate(const std::source_location& location, const char* message) {
  return std::string(location.file_name()) + ":" + std::to_string(location.line()) + " " + message;
}
