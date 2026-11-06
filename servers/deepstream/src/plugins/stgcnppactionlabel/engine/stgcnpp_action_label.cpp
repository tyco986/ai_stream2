#include "plugins/stgcnppactionlabel/engine/stgcnpp_action_label.hpp"

#include "constants/stgcnppactionlabel_const.hpp"
#include "plugins/nvdspreprocess/stgcnpppreprocess/engine/stgcnpp_ready_meta.hpp"

#include <cstring>

extern "C" gpointer stgcnppActionMetaCopy(gpointer data, gpointer userData);
extern "C" void stgcnppActionMetaRelease(gpointer data, gpointer userData);

StgcnppActionLabel::StgcnppActionLabel(int classifierUniqueId)
    : uniqueId(classifierUniqueId) {
}

void StgcnppActionLabel::setClassifierUniqueId(int classifierUniqueId) {
    uniqueId = classifierUniqueId;
}

int StgcnppActionLabel::classifierUniqueId() const {
    return uniqueId;
}

const StgcnppActionMeta* StgcnppActionLabel::fromObject(NvDsObjectMeta* object) const {
    const StgcnppActionMeta* meta = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == stgcnppactionlabel::USER_META) {
                meta = static_cast<const StgcnppActionMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return meta;
}

int StgcnppActionLabel::objectReady(NvDsObjectMeta* object) const {
    int ready = FALSE;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == stgcnpp::READY_USER_META) {
                auto* meta = static_cast<StgcnppReadyMeta*>(userMeta->user_meta_data);
                if (meta != nullptr) {
                    ready = meta->ready;
                }
            }
        }
    }
    return ready;
}

void StgcnppActionLabel::classifierResult(
    NvDsObjectMeta* object, const char** name, float* score, int* classId) const {
    const char* label = nullptr;
    float conf = stgcnppactionlabel::CONF_NONE;
    int cls = stgcnppactionlabel::CLASS_NONE;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->classifier_meta_list; item != nullptr; item = item->next) {
            auto* classifier = static_cast<NvDsClassifierMeta*>(item->data);
            if (classifier != nullptr && static_cast<int>(classifier->unique_component_id) == uniqueId) {
                for (NvDsMetaList* labelItem = classifier->label_info_list; labelItem != nullptr; labelItem = labelItem->next) {
                    auto* info = static_cast<NvDsLabelInfo*>(labelItem->data);
                    if (info != nullptr) {
                        label = (info->pResult_label != nullptr && info->pResult_label[0] != '\0')
                            ? info->pResult_label
                            : info->result_label;
                        conf = info->result_prob;
                        cls = static_cast<int>(info->result_class_id);
                    }
                }
            }
        }
    }
    if (name != nullptr) {
        *name = label;
    }
    if (score != nullptr) {
        *score = conf;
    }
    if (classId != nullptr) {
        *classId = cls;
    }
}

void StgcnppActionLabel::writeObject(NvDsBatchMeta* batch, NvDsObjectMeta* object) {
    const int ready = objectReady(object);
    const char* cls = nullptr;
    float clsConf = stgcnppactionlabel::CONF_NONE;
    int clsId = stgcnppactionlabel::CLASS_NONE;
    if (ready) {
        classifierResult(object, &cls, &clsConf, &clsId);
    }
    const bool labeled = ready && cls != nullptr && cls[0] != '\0';
    const char* label = ready ? (labeled ? cls : stgcnppactionlabel::LABEL_UNKNOWN) : stgcnppactionlabel::LABEL_NA;
    const float conf = labeled ? clsConf : stgcnppactionlabel::CONF_NONE;
    const int classId = labeled ? clsId : stgcnppactionlabel::CLASS_NONE;
    StgcnppActionMeta* meta = const_cast<StgcnppActionMeta*>(fromObject(object));
    if (meta == nullptr && batch != nullptr && object != nullptr) {
        NvDsUserMeta* userMeta = nvds_acquire_user_meta_from_pool(batch);
        if (userMeta != nullptr) {
            meta = static_cast<StgcnppActionMeta*>(g_malloc0(sizeof(StgcnppActionMeta)));
            userMeta->user_meta_data = meta;
            userMeta->base_meta.meta_type = stgcnppactionlabel::USER_META;
            userMeta->base_meta.copy_func = stgcnppActionMetaCopy;
            userMeta->base_meta.release_func = stgcnppActionMetaRelease;
            nvds_add_user_meta_to_obj(object, userMeta);
        }
    }
    if (meta != nullptr) {
        meta->conf = conf;
        meta->classId = classId;
        meta->ready = ready;
        g_strlcpy(meta->label, label, sizeof(meta->label));
    }
}

void StgcnppActionLabel::attach(NvDsBatchMeta* batch) {
    if (batch != nullptr) {
        for (NvDsMetaList* frameItem = batch->frame_meta_list; frameItem != nullptr; frameItem = frameItem->next) {
            auto* frame = static_cast<NvDsFrameMeta*>(frameItem->data);
            if (frame != nullptr) {
                for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
                    auto* object = static_cast<NvDsObjectMeta*>(item->data);
                    if (object != nullptr) {
                        writeObject(batch, object);
                    }
                }
            }
        }
    }
}

StgcnppActionMeta* StgcnppActionLabel::copy(const StgcnppActionMeta* src) {
    StgcnppActionMeta* dst = nullptr;
    if (src != nullptr) {
        dst = static_cast<StgcnppActionMeta*>(g_malloc0(sizeof(StgcnppActionMeta)));
        std::memcpy(dst, src, sizeof(StgcnppActionMeta));
    }
    return dst;
}

void StgcnppActionLabel::release(StgcnppActionMeta* meta) {
    g_free(meta);
}

extern "C" gpointer stgcnppActionMetaCopy(gpointer data, gpointer userData) {
    (void)userData;
    const StgcnppActionMeta* src = nullptr;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        src = static_cast<const StgcnppActionMeta*>(userMeta->user_meta_data);
    }
    return StgcnppActionLabel::copy(src);
}

extern "C" void stgcnppActionMetaRelease(gpointer data, gpointer userData) {
    (void)userData;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        StgcnppActionLabel::release(static_cast<StgcnppActionMeta*>(userMeta->user_meta_data));
        userMeta->user_meta_data = nullptr;
    }
}
