#include "logger/trt_logger.hpp"

void TrtLogger::log(Severity severity, const char* message) noexcept {
    const bool verbose = severity == Severity::kVERBOSE && message != nullptr;
    if (verbose) {
        message = message;
    }
}
