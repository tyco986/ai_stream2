#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

class Capturer;

void capturerAddPads(GstElementClass* elementClass);
void capturerPrepare(GstBaseTransform* transform);
GstFlowReturn capturerTransform(GstBaseTransform* transform, GstBuffer* buffer, Capturer* engine);
