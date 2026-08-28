#include <gst/gst.h>

#include "gstnvobjectcoder.h"
#include "gstnvpresencecoder.h"
#include "gstnveventcoder_common.h"

static gboolean
nveventcoder_plugin_init(GstPlugin *plugin)
{
  gboolean ok = TRUE;
  ok = ok && gst_element_register(plugin, "nvpresencecoder", GST_RANK_PRIMARY,
                                  GST_TYPE_NVPRESENCECODER);
  ok = ok && gst_element_register(plugin, "nvobjectcoder", GST_RANK_PRIMARY,
                                  GST_TYPE_NVOBJECTCODER);
  return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvdsgst_eventcoder,
                  "DeepStream event coder elements",
                  nveventcoder_plugin_init,
                  NVEVENTCODER_VERSION,
                  NVEVENTCODER_LICENSE,
                  NVEVENTCODER_BINARY,
                  NVEVENTCODER_URL)
