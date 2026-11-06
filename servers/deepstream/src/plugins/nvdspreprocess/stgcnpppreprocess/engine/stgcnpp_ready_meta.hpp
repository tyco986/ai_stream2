#pragma once

#include <glib.h>

#include "constants/stgcnpp_const.hpp"
#include "nvdsmeta.h"

struct StgcnppReadyMeta {
    gboolean ready;
    gint length;
};

class StgcnppReadyMetaStore {
public:
    static StgcnppReadyMeta* fromObject(NvDsObjectMeta* object);
    static void write(NvDsBatchMeta* batch, NvDsObjectMeta* object, gboolean ready, gint length);
    static StgcnppReadyMeta* copy(const StgcnppReadyMeta* src);
    static void release(StgcnppReadyMeta* meta);
};
