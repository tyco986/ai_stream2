#pragma once

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/log_engine.hpp"

class StgcnppLogEngine : public LogEngine {
public:
    explicit StgcnppLogEngine(int interval);

protected:
    const char* page() const override;
    void appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const override;

private:
    struct ActionMeta {
        float conf;
        int classId;
        int ready;
        char label[MAX_LABEL_SIZE];
    };

    struct ReadyMeta {
        int ready;
        int length;
    };

    const ActionMeta* actionMeta(NvDsObjectMeta* object) const;
    const ReadyMeta* readyMeta(NvDsObjectMeta* object) const;
};
