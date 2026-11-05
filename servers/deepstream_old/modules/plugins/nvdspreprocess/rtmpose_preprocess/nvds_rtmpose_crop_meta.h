#pragma once

#include <glib.h>

#include "nvdsmeta.h"

#define NVDS_RTMPOSE_CROP_USER_META ((NvDsMetaType)(NVDS_START_USER_META + 0x5254))

typedef struct {
  gint src_left;
  gint src_top;
  gint src_width;
  gint src_height;
  gint dest_width;
  gint dest_height;
  gint offset_left;
  gint offset_top;
  gint infer_width;
  gint infer_height;
} NvDsRtmposeCropMeta;

#ifdef __cplusplus
extern "C" {
#endif

gpointer nvds_rtmpose_crop_meta_copy(gpointer data, gpointer user_data);
void nvds_rtmpose_crop_meta_release(gpointer data, gpointer user_data);

#ifdef __cplusplus
}
#endif
