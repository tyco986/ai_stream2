#include <gst/gst.h>

#include "constants/collector_const.hpp"
#include "plugins/dscollector/gst_bridge/gst_det_collector.hpp"
#include "plugins/dscollector/gst_bridge/gst_presence_collector.hpp"
#include "plugins/dscollector/gst_bridge/gst_stgcnpp_collector.hpp"

static gboolean dscollectorPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(plugin, collector::ELEMENT_DET, GST_RANK_PRIMARY, GST_TYPE_DET_COLLECTOR)
        && gst_element_register(plugin, collector::ELEMENT_PRESENCE, GST_RANK_PRIMARY, GST_TYPE_PRESENCE_COLLECTOR)
        && gst_element_register(plugin, collector::ELEMENT_STGCNPP, GST_RANK_PRIMARY, GST_TYPE_STGCNPP_COLLECTOR);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  dscollector,
                  collector::PLUGIN_DESCRIPTION,
                  dscollectorPluginInit,
                  collector::VERSION,
                  collector::LICENSE,
                  collector::PLUGIN_PACKAGE,
                  collector::ORIGIN)
