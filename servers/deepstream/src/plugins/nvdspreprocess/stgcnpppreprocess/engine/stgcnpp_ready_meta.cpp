#include "plugins/nvdspreprocess/stgcnpppreprocess/engine/stgcnpp_ready_meta.hpp"

#include <cstring>

extern "C" gpointer stgcnppReadyMetaCopy(gpointer data, gpointer userData);
extern "C" void stgcnppReadyMetaRelease(gpointer data, gpointer userData);

StgcnppReadyMeta* StgcnppReadyMetaStore::fromObject(NvDsObjectMeta* object) {
    StgcnppReadyMeta* meta = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == stgcnpp::READY_USER_META) {
                meta = static_cast<StgcnppReadyMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return meta;
}

void StgcnppReadyMetaStore::write(NvDsBatchMeta* batch, NvDsObjectMeta* object, gboolean ready, gint length) {
    StgcnppReadyMeta* meta = fromObject(object);
    if (meta == nullptr && batch != nullptr && object != nullptr) {
        NvDsUserMeta* userMeta = nvds_acquire_user_meta_from_pool(batch);
        if (userMeta != nullptr) {
            meta = static_cast<StgcnppReadyMeta*>(g_malloc0(sizeof(StgcnppReadyMeta)));
            userMeta->user_meta_data = meta;
            userMeta->base_meta.meta_type = stgcnpp::READY_USER_META;
            userMeta->base_meta.copy_func = stgcnppReadyMetaCopy;
            userMeta->base_meta.release_func = stgcnppReadyMetaRelease;
            nvds_add_user_meta_to_obj(object, userMeta);
        }
    }
    if (meta != nullptr) {
        meta->ready = ready;
        meta->length = length;
    }
}

StgcnppReadyMeta* StgcnppReadyMetaStore::copy(const StgcnppReadyMeta* src) {
    StgcnppReadyMeta* dst = nullptr;
    if (src != nullptr) {
        dst = static_cast<StgcnppReadyMeta*>(g_malloc0(sizeof(StgcnppReadyMeta)));
        std::memcpy(dst, src, sizeof(StgcnppReadyMeta));
    }
    return dst;
}

void StgcnppReadyMetaStore::release(StgcnppReadyMeta* meta) {
    g_free(meta);
}

extern "C" gpointer stgcnppReadyMetaCopy(gpointer data, gpointer userData) {
    (void)userData;
    const StgcnppReadyMeta* src = nullptr;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        src = static_cast<const StgcnppReadyMeta*>(userMeta->user_meta_data);
    }
    return StgcnppReadyMetaStore::copy(src);
}

extern "C" void stgcnppReadyMetaRelease(gpointer data, gpointer userData) {
    (void)userData;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        StgcnppReadyMetaStore::release(static_cast<StgcnppReadyMeta*>(userMeta->user_meta_data));
        userMeta->user_meta_data = nullptr;
    }
}
