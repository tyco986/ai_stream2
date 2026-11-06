#include "constants/boxsnapshot_const.hpp"
#include "plugins/boxsnapshot/gst_bridge/gst_boxsnapshot.hpp"

#include <gst/gst.h>

static gboolean boxsnapshotPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(
        plugin, boxsnapshot::ELEMENT_NAME, GST_RANK_PRIMARY, GST_TYPE_BOXSNAPSHOT);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  boxsnapshot,
                  boxsnapshot::PLUGIN_DESCRIPTION,
                  boxsnapshotPluginInit,
                  boxsnapshot::VERSION,
                  boxsnapshot::LICENSE,
                  boxsnapshot::PLUGIN_PACKAGE,
                  boxsnapshot::ORIGIN)
