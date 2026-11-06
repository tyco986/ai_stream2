#include "constants/stgcnppactionlabel_const.hpp"
#include "plugins/stgcnppactionlabel/gst_bridge/gst_stgcnpp_action_label.hpp"

#include <gst/gst.h>

static gboolean stgcnppactionlabelPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(
        plugin, stgcnppactionlabel::ELEMENT_NAME, GST_RANK_PRIMARY, GST_TYPE_STGCNPP_ACTION_LABEL);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  stgcnppactionlabel,
                  stgcnppactionlabel::PLUGIN_DESCRIPTION,
                  stgcnppactionlabelPluginInit,
                  stgcnppactionlabel::VERSION,
                  stgcnppactionlabel::LICENSE,
                  stgcnppactionlabel::PLUGIN_PACKAGE,
                  stgcnppactionlabel::ORIGIN)
