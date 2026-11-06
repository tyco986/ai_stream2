#include "constants/presenceeventcoder_const.hpp"
#include "plugins/presenceeventcoder/gst_bridge/gst_presence_event_coder.hpp"

#include <gst/gst.h>

static gboolean presenceeventcoderPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(
        plugin, presenceeventcoder::ELEMENT_NAME, GST_RANK_PRIMARY, GST_TYPE_PRESENCE_EVENT_CODER);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  presenceeventcoder,
                  presenceeventcoder::PLUGIN_DESCRIPTION,
                  presenceeventcoderPluginInit,
                  presenceeventcoder::VERSION,
                  presenceeventcoder::LICENSE,
                  presenceeventcoder::PLUGIN_PACKAGE,
                  presenceeventcoder::ORIGIN)
