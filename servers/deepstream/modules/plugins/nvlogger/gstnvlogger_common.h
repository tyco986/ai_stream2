#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#define NVLOGGER_PACKAGE "nvlogger"
#define NVLOGGER_VERSION "1.0"
#define NVLOGGER_LICENSE "Apache-2.0"
#define NVLOGGER_BINARY "DeepStream object loggers"
#define NVLOGGER_URL "https://github.com"

#define NVLOGGER_NVMM_CAPS \
  GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")

#define NVLOGGER_PLACEHOLDER_ROOT "/tmp/ds-detlog"
#define NVLOGGER_MAX_BYTES (1024 * 1024)

#define NVLOGGER_LOG_HEADER \
  "# object item: [x1, y1, x2, y2, conf, cls, label, id]\n" \
  "# line: {pad, source, frame, object}\n"
