#include "logger/trt_logger.hpp"

void TrtLogger::log(Severity severity, const char* message) noexcept {
    const bool kept = message != nullptr && severity <= Severity::kWARNING;
    if (kept) {
        lines += message;
        lines += '\n';
    }
}

const std::string& TrtLogger::errors() const {
    return lines;
}
