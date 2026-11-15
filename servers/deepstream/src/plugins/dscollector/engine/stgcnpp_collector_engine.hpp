#pragma once

#include "plugins/dscollector/engine/collector_engine.hpp"
#include "plugins/nvdspreprocess/stgcnpppreprocess/engine/stgcnpp_ready_meta.hpp"
#include "plugins/stgcnppactionlabel/engine/stgcnpp_action_label.hpp"

class StgcnppCollectorEngine : public CollectorEngine {
public:
    StgcnppCollectorEngine() = default;

protected:
    void appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const override;

private:
    const StgcnppActionMeta* actionMeta(NvDsObjectMeta* object) const;
    const StgcnppReadyMeta* readyMeta(NvDsObjectMeta* object) const;
};
