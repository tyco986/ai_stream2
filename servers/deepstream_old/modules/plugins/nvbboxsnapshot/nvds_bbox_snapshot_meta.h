#pragma once

#include <glib.h>

#include "nvdsmeta.h"

#define NVDS_BBOX_SNAPSHOT_USER_META ((NvDsMetaType)(NVDS_START_USER_META + 0x4253))

typedef struct {
  gfloat left;
  gfloat top;
  gfloat width;
  gfloat height;
  gfloat confidence;
  gint class_id;
  guint64 object_id;
  gchar label[64];
  gfloat *mask;
  guint mask_size;
  guint mask_width;
  guint mask_height;
  gfloat mask_threshold;
} NvDsBboxSnapshotBox;

typedef struct {
  guint num_boxes;
  NvDsBboxSnapshotBox *boxes;
} NvDsBboxSnapshotMeta;

#ifdef __cplusplus
extern "C" {
#endif

gpointer nvds_bbox_snapshot_meta_copy(gpointer data, gpointer user_data);
void nvds_bbox_snapshot_meta_release(gpointer data, gpointer user_data);

#ifdef __cplusplus
}
#endif
