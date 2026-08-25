#include <gst/gst.h>

#include "gstnvdetlogger.h"
#include "gstnvlogger_common.h"

static gboolean
nvlogger_plugin_init(GstPlugin *plugin)
{
  gboolean ok = gst_element_register(plugin, "nvdetlogger", GST_RANK_PRIMARY,
                                     GST_TYPE_NVDETLOGGER);
  return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvdsgst_logger,
                  "DeepStream object logger elements",
                  nvlogger_plugin_init,
                  NVLOGGER_VERSION,
                  NVLOGGER_LICENSE,
                  NVLOGGER_BINARY,
                  NVLOGGER_URL)
