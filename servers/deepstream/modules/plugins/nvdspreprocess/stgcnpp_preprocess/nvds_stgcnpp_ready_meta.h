#pragma once

#include <glib.h>

#include "nvdsmeta.h"

#define NVDS_STGCNPP_READY_USER_META ((NvDsMetaType)(NVDS_START_USER_META + 0x5347))

typedef struct {
  gboolean ready;
  gint length;
} NvDsStgcnppReadyMeta;

#ifdef __cplusplus
extern "C" {
#endif

gpointer nvds_stgcnpp_ready_meta_copy(gpointer data, gpointer user_data);
void nvds_stgcnpp_ready_meta_release(gpointer data, gpointer user_data);

#ifdef __cplusplus
}

static inline NvDsStgcnppReadyMeta *nvds_stgcnpp_ready_meta_from_obj(NvDsObjectMeta *obj)
{
  NvDsStgcnppReadyMeta *meta = nullptr;
  if (obj != nullptr) {
    for (NvDsMetaList *item = obj->obj_user_meta_list; item != nullptr; item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta != nullptr &&
          user_meta->base_meta.meta_type == NVDS_STGCNPP_READY_USER_META) {
        meta = static_cast<NvDsStgcnppReadyMeta *>(user_meta->user_meta_data);
      }
    }
  }
  return meta;
}
#endif
