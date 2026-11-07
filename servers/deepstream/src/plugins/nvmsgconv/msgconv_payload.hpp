#pragma once

#include <memory>
#include <string>

#include "nvdsmeta.h"
#include "plugins/dslogger/engine/log_engine.hpp"

class MsgconvPayload {
public:
    MsgconvPayload();

    bool publishable(NvDsFrameMeta* frame) const;
    std::string line(NvDsFrameMeta* frame) const;

private:
    std::unique_ptr<LogEngine> engine;
};
