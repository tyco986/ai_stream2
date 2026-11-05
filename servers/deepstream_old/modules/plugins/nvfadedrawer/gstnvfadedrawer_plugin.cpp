#include <gst/gst.h>

#include "gstnvdetfadedrawer.h"
#include "gstnvdetfadedrawerwithtracker.h"
#include "gstnvfadedrawer_common.h"
#include "gstnvposefadedrawer.h"
#include "gstnvposefadedrawerwithtracker.h"
#include "gstnvstgcnppfadedrawerwithtracker.h"
#include "gstnvsegfadedrawer.h"
#include "gstnvsegfadedrawerwithtracker.h"

static gboolean
nvfadedrawer_plugin_init(GstPlugin *plugin)
{
  gboolean ok = TRUE;
  ok = ok && gst_element_register(plugin, "nvdetfadedrawer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVDETFADEDRAWER);
  ok = ok && gst_element_register(plugin, "nvdetfadedrawerwithtracker", GST_RANK_PRIMARY,
                                  GST_TYPE_NVDETFADEDRAWERWITHTRACKER);
  ok = ok && gst_element_register(plugin, "nvsegfadedrawer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVSEGFADEDRAWER);
  ok = ok && gst_element_register(plugin, "nvsegfadedrawerwithtracker", GST_RANK_PRIMARY,
                                  GST_TYPE_NVSEGFADEDRAWERWITHTRACKER);
  ok = ok && gst_element_register(plugin, "nvposefadedrawer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVPOSEFADEDRAWER);
  ok = ok && gst_element_register(plugin, "nvposefadedrawerwithtracker", GST_RANK_PRIMARY,
                                  GST_TYPE_NVPOSEFADEDRAWERWITHTRACKER);
  ok = ok && gst_element_register(
      plugin, "nvstgcnppfadedrawerwithtracker", GST_RANK_PRIMARY,
      GST_TYPE_NVSTGCNPPFADEDRAWERWITHTRACKER);
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
