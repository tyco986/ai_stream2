#include <gst/gst.h>

#include "gstnvrtmposepostprocess.h"

#define NVRTMPOSEPOSTPROCESS_VERSION "1.0"
#define NVRTMPOSEPOSTPROCESS_LICENSE "Apache-2.0"
#define NVRTMPOSEPOSTPROCESS_BINARY "DeepStream RTMPose postprocess"
#define NVRTMPOSEPOSTPROCESS_URL "https://github.com"

static gboolean
nvrtmposepostprocess_plugin_init(GstPlugin *plugin)
{
  gboolean ok = gst_element_register(
      plugin, "nvrtmposepostprocess", GST_RANK_PRIMARY, GST_TYPE_NVRTMPOSEPOSTPROCESS);
  return ok;
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    nvdsgst_rtmposepostprocess,
    "DeepStream RTMPose crop keypoints to mask_params",
    nvrtmposepostprocess_plugin_init,
    NVRTMPOSEPOSTPROCESS_VERSION,
    NVRTMPOSEPOSTPROCESS_LICENSE,
    NVRTMPOSEPOSTPROCESS_BINARY,
    NVRTMPOSEPOSTPROCESS_URL)
