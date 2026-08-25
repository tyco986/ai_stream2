#pragma once

#include "nvdsmeta.h"
#include "nvfadedrawer_constants.h"

#define NVDS_FADE_EVENT_USER_META ((NvDsMetaType)(NVDS_START_USER_META + 0x4644))

typedef struct {
  char event_codes[nvfadedrawer::kEventCodeLen + 1];
} NvDsFadeEventMeta;
