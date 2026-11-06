#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/det_log_engine.hpp"

DetLogEngine::DetLogEngine(int interval)
    : LogEngine(interval) {
}

const char* DetLogEngine::page() const {
    return dslogger::PAGE_DET;
}
