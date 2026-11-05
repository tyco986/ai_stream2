#include <gst/gst.h>

#include "gstnvcapturer_common.h"
#include "gstnvrawcapturer.h"
#include "gstnvviscapturer.h"

static gboolean
nvcapturer_plugin_init(GstPlugin *plugin)
{
  gboolean ok = TRUE;
  ok = ok && gst_element_register(plugin, "nvrawcapturer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVRAWCAPTURER);
  ok = ok && gst_element_register(plugin, "nvviscapturer", GST_RANK_PRIMARY,
                                  GST_TYPE_NVVISCAPTURER);
  return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvdsgst_capturer,
                  "DeepStream raw/vis capture elements",
                  nvcapturer_plugin_init,
                  NVCAPTURER_VERSION,
                  NVCAPTURER_LICENSE,
                  NVCAPTURER_BINARY,
                  NVCAPTURER_URL)
