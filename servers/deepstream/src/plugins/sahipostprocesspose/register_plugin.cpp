#include "constants/sahipostprocesspose_const.hpp"
#include "plugins/sahipostprocesspose/gst_bridge/gst_sahi_postprocess_pose.hpp"

#include <gst/gst.h>

static gboolean sahipostprocessposePluginInit(GstPlugin* plugin) {
    const gboolean registered = gst_element_register(
        plugin, sahipostprocesspose::ELEMENT_NAME, GST_RANK_PRIMARY, GST_TYPE_SAHI_POSTPROCESS_POSE);
    return registered;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  nvsahipostprocesspose,
                  sahipostprocesspose::PLUGIN_DESCRIPTION,
                  sahipostprocessposePluginInit,
                  sahipostprocesspose::VERSION,
                  sahipostprocesspose::LICENSE,
                  sahipostprocesspose::PLUGIN_PACKAGE,
                  sahipostprocesspose::ORIGIN)
