#include "plugins/dscollector/engine/stgcnpp_collector_engine.hpp"

#include <cmath>
#include <iomanip>

#include "constants/collector_const.hpp"
#include "constants/stgcnpp_const.hpp"
#include "constants/stgcnppactionlabel_const.hpp"

const StgcnppActionMeta* StgcnppCollectorEngine::actionMeta(NvDsObjectMeta* object) const {
    const StgcnppActionMeta* action = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr && action == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == stgcnppactionlabel::USER_META) {
                action = static_cast<const StgcnppActionMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return action;
}

const StgcnppReadyMeta* StgcnppCollectorEngine::readyMeta(NvDsObjectMeta* object) const {
    const StgcnppReadyMeta* ready = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr && ready == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == stgcnpp::READY_USER_META) {
                ready = static_cast<const StgcnppReadyMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return ready;
}

void StgcnppCollectorEngine::appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const {
    float conf = stgcnppactionlabel::CONF_NONE;
    int classId = stgcnppactionlabel::CLASS_NONE;
    const char* label = "";
    bool ready = false;
    int length = 0;
    const StgcnppActionMeta* action = actionMeta(object);
    if (action != nullptr) {
        conf = action->conf;
        classId = action->classId;
        label = action->label;
        ready = action->ready != 0;
    }
    const StgcnppReadyMeta* readyState = readyMeta(object);
    if (readyState != nullptr) {
        length = readyState->length;
    }
    const double logged = std::round(static_cast<double>(conf) * collector::CONF_SCALE) / collector::CONF_SCALE;
    json << "{\"det\":[";
    appendDetFields(json, object);
    json << "],\"action\":[" << std::fixed << std::setprecision(collector::CONF_DIGITS) << logged << ","
         << classId << ",\"" << escapeLabel(label) << "\","
         << (ready ? "true" : "false") << "," << length << "]}";
}
