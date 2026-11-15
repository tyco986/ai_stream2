#pragma once

#include "constants/collector_const.hpp"
#include "nvdsmeta.h"

inline const char* collectorLine(NvDsFrameMeta* frame) {
    const char* line = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && line == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == collector::USER_META && userMeta->user_meta_data != nullptr) {
                line = static_cast<const char*>(userMeta->user_meta_data);
            }
        }
    }
    return line;
}
