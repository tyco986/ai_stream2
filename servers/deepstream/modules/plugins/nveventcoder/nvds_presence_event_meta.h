#pragma once

#include <glib.h>

#include "nvdsmeta.h"

#define NVDS_PRESENCE_EVENT_USER_META ((NvDsMetaType)(NVDS_START_USER_META + 0x5045))

#define NVDS_PRESENCE_EVENT_CODE_LEN 8
#define NVDS_PRESENCE_EVENT_NAME_LEN 64
#define NVDS_PRESENCE_EVENT_CODE_BOUNDS 3

typedef struct {
  gint class_id;
  guint counts[NVDS_PRESENCE_EVENT_CODE_BOUNDS];
  gfloat ratio;
} NvDsPresenceEventClassStat;

typedef struct {
  gchar event_codes[NVDS_PRESENCE_EVENT_CODE_LEN + 1];
  gchar event_names[NVDS_PRESENCE_EVENT_CODE_LEN][NVDS_PRESENCE_EVENT_NAME_LEN];
  guint num_classes;
  NvDsPresenceEventClassStat classes[NVDS_PRESENCE_EVENT_CODE_LEN];
} NvDsPresenceEventMeta;

#ifdef __cplusplus
extern "C" {
#endif

gpointer nvds_presence_event_meta_copy(gpointer data, gpointer user_data);
void nvds_presence_event_meta_release(gpointer data, gpointer user_data);

#ifdef __cplusplus
}
#endif
