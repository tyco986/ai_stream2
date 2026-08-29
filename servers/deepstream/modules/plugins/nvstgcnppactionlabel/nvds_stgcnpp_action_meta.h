#pragma once

#include <glib.h>

#include "nvdsmeta.h"

#define NVDS_STGCNPP_ACTION_USER_META ((NvDsMetaType)(NVDS_START_USER_META + 0x5341))
#define NVDS_STGCNPP_ACTION_CONF_NONE 0.0f
#define NVDS_STGCNPP_ACTION_CLASS_NONE -1

typedef struct {
  gfloat conf;
  gint class_id;
  gboolean ready;
  gchar label[MAX_LABEL_SIZE];
} NvDsStgcnppActionMeta;

#ifdef __cplusplus
extern "C" {
#endif

gpointer nvds_stgcnpp_action_meta_copy(gpointer data, gpointer user_data);
void nvds_stgcnpp_action_meta_release(gpointer data, gpointer user_data);

#ifdef __cplusplus
}

static inline NvDsStgcnppActionMeta *nvds_stgcnpp_action_meta_from_obj(NvDsObjectMeta *obj)
{
  NvDsStgcnppActionMeta *meta = nullptr;
  if (obj != nullptr) {
    for (NvDsMetaList *item = obj->obj_user_meta_list; item != nullptr; item = item->next) {
      auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
      if (user_meta != nullptr &&
          user_meta->base_meta.meta_type == NVDS_STGCNPP_ACTION_USER_META) {
        meta = static_cast<NvDsStgcnppActionMeta *>(user_meta->user_meta_data);
      }
    }
  }
  return meta;
}
#endif
