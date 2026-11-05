#include <gst/gst.h>

#include "gstnvstgcnppactionlabel.h"

#define NVSTGCNPPACTIONLABEL_VERSION "1.0"
#define NVSTGCNPPACTIONLABEL_LICENSE "Apache-2.0"
#define NVSTGCNPPACTIONLABEL_BINARY "DeepStream ST-GCN++ action label"
#define NVSTGCNPPACTIONLABEL_URL "https://github.com"

static gboolean
nvstgcnppactionlabel_plugin_init(GstPlugin *plugin)
{
  gboolean ok = gst_element_register(
      plugin, "nvstgcnppactionlabel", GST_RANK_PRIMARY, GST_TYPE_NVSTGCNPPACTIONLABEL);
  return ok;
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    nvdsgst_stgcnppactionlabel,
    "Write ST-GCN++ action labels onto object metadata",
    nvstgcnppactionlabel_plugin_init,
    NVSTGCNPPACTIONLABEL_VERSION,
    NVSTGCNPPACTIONLABEL_LICENSE,
    NVSTGCNPPACTIONLABEL_BINARY,
    NVSTGCNPPACTIONLABEL_URL)
