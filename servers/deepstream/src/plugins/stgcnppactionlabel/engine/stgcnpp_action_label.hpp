#pragma once

#include <glib.h>

#include "gstnvdsmeta.h"

struct StgcnppActionMeta {
    float conf;
    int classId;
    int ready;
    char label[MAX_LABEL_SIZE];
};

class StgcnppActionLabel {
public:
    explicit StgcnppActionLabel(int classifierUniqueId);

    void setClassifierUniqueId(int classifierUniqueId);
    int classifierUniqueId() const;
    void attach(NvDsBatchMeta* batch);

    static StgcnppActionMeta* copy(const StgcnppActionMeta* src);
    static void release(StgcnppActionMeta* meta);

private:
    const StgcnppActionMeta* fromObject(NvDsObjectMeta* object) const;
    int objectReady(NvDsObjectMeta* object) const;
    void classifierResult(NvDsObjectMeta* object, const char** name, float* score, int* classId) const;
    void writeObject(NvDsBatchMeta* batch, NvDsObjectMeta* object);

    int uniqueId;
};
