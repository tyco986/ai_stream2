#include <gst/gst.h>

#include "gstnvbboxsnapshot.h"

#define NVBBOXSNAPSHOT_VERSION "1.0"
#define NVBBOXSNAPSHOT_LICENSE "Apache-2.0"
#define NVBBOXSNAPSHOT_BINARY "DeepStream bbox snapshot"
#define NVBBOXSNAPSHOT_URL "https://github.com"

static gboolean
nvbboxsnapshot_plugin_init(GstPlugin *plugin)
{
  gboolean ok = gst_element_register(plugin, "nvbboxsnapshot", GST_RANK_PRIMARY,
                                     GST_TYPE_NVBBOXSNAPSHOT);
  return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvdsgst_bboxsnapshot,
                  "DeepStream frame bbox snapshot",
                  nvbboxsnapshot_plugin_init,
                  NVBBOXSNAPSHOT_VERSION,
                  NVBBOXSNAPSHOT_LICENSE,
                  NVBBOXSNAPSHOT_BINARY,
                  NVBBOXSNAPSHOT_URL)
