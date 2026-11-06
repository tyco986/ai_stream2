#pragma once

#include "plugins/dslogger/engine/log_engine.hpp"

class DetLogEngine : public LogEngine {
public:
    explicit DetLogEngine(int interval);

protected:
    const char* page() const override;
};
