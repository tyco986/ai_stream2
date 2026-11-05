#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#define NVFADEDRAWER_PACKAGE "nvfadedrawer"
#define NVFADEDRAWER_VERSION "1.0"
#define NVFADEDRAWER_LICENSE "Apache-2.0"
#define NVFADEDRAWER_BINARY "DeepStream fade drawer"
#define NVFADEDRAWER_URL "https://github.com"

#define NVFADEDRAWER_NVMM_CAPS \
  GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
