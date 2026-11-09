#include "constants/rtmposepostprocess_const.hpp"
#include "plugins/rtmposepostprocess/gst_bridge/gst_rtmpose_postprocess.hpp"

#include <gst/gst.h>

static gboolean rtmposepostprocessPluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(
        plugin, rtmposepostprocess::ELEMENT_NAME, GST_RANK_PRIMARY, GST_TYPE_RTMPOSE_POSTPROCESS);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  rtmposepostprocess,
                  rtmposepostprocess::PLUGIN_DESCRIPTION,
                  rtmposepostprocessPluginInit,
                  rtmposepostprocess::VERSION,
                  rtmposepostprocess::LICENSE,
                  rtmposepostprocess::PLUGIN_PACKAGE,
                  rtmposepostprocess::ORIGIN)
