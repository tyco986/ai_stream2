#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#define NVCAPTURER_PACKAGE "nvcapturer"
#define NVCAPTURER_VERSION "1.0"
#define NVCAPTURER_LICENSE "Apache-2.0"
#define NVCAPTURER_BINARY "DeepStream capture"
#define NVCAPTURER_URL "https://github.com"

#define NVCAPTURER_NVMM_CAPS \
  GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ RGB, RGBA }")

#define NVCAPTURER_DEFAULT_OUTPUT_DIR "/root/output"
#define NVCAPTURER_DEFAULT_CAPTURE_CODES "1"
#define NVCAPTURER_DEFAULT_LABEL_TASK "det"
