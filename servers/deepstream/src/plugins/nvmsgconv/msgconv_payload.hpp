#pragma once

#include <string>

#include "nvdsmeta.h"

class MsgconvPayload {
public:
    bool publishable(NvDsFrameMeta* frame) const;
    std::string line(NvDsFrameMeta* frame) const;
};
