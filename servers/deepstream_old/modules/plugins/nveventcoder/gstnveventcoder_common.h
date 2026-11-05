#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#define NVEVENTCODER_PACKAGE "nveventcoder"
#define NVEVENTCODER_VERSION "1.0"
#define NVEVENTCODER_LICENSE "Apache-2.0"
#define NVEVENTCODER_BINARY "DeepStream event coder"
#define NVEVENTCODER_URL "https://github.com"

#define NVEVENTCODER_NVMM_CAPS \
  GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
