#include <cmath>
#include <iomanip>

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/stgcnpp_log_engine.hpp"

StgcnppLogEngine::StgcnppLogEngine(int interval)
    : LogEngine(interval) {
}

const char* StgcnppLogEngine::page() const {
    return dslogger::PAGE_STGCNPP;
}

const StgcnppLogEngine::ActionMeta* StgcnppLogEngine::actionMeta(NvDsObjectMeta* object) const {
    const ActionMeta* action = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr && action == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == dslogger::STGCNPP_ACTION_USER_META) {
                action = static_cast<const ActionMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return action;
}

const StgcnppLogEngine::ReadyMeta* StgcnppLogEngine::readyMeta(NvDsObjectMeta* object) const {
    const ReadyMeta* ready = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr && ready == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == dslogger::STGCNPP_READY_USER_META) {
                ready = static_cast<const ReadyMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return ready;
}

void StgcnppLogEngine::appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const {
    float conf = dslogger::ACTION_CONF_NONE;
    int classId = dslogger::ACTION_CLASS_NONE;
    const char* label = "";
    bool ready = false;
    int length = 0;
    const ActionMeta* action = actionMeta(object);
    if (action != nullptr) {
        conf = action->conf;
        classId = action->classId;
        label = action->label;
        ready = action->ready != 0;
    }
    const ReadyMeta* readyMeta = this->readyMeta(object);
    if (readyMeta != nullptr) {
        length = readyMeta->length;
    }
    const double logged = std::round(static_cast<double>(conf) * dslogger::CONF_SCALE) / dslogger::CONF_SCALE;
    json << "{\"det\":[";
    appendDetFields(json, object);
    json << "],\"action\":[" << std::fixed << std::setprecision(dslogger::CONF_DIGITS) << logged << ","
         << classId << ",\"" << escapeLabel(label) << "\","
         << (ready ? "true" : "false") << "," << length << "]}";
}
