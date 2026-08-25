#include <gst/gst.h>

#include "gstnvdetfadedrawer.h"
#include "gstnvfadedrawer_common.h"
#include "gstnvposefadedrawer.h"
#include "gstnvsegfadedrawer.h"

static gboolean
nvfadedrawer_plugin_init(GstPlugin *plugin)
{
  gboolean ok = TRUE;
  ok = ok && gst_element_register(plugin, "nvdetfadedrawer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVDETFADEDRAWER);
  ok = ok && gst_element_register(plugin, "nvsegfadedrawer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVSEGFADEDRAWER);
  ok = ok && gst_element_register(plugin, "nvposefadedrawer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVPOSEFADEDRAWER);
  return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvdsgst_fadedrawer,
                  "DeepStream fade drawer elements",
                  nvfadedrawer_plugin_init,
                  NVFADEDRAWER_VERSION,
                  NVFADEDRAWER_LICENSE,
                  NVFADEDRAWER_BINARY,
                  NVFADEDRAWER_URL)
