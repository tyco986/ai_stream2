#pragma once

#include <gst/base/gstbasetransform.h>

class LogEngine;

void dsloggerAddPads(GstElementClass* elementClass);
void dsloggerPrepare(GstBaseTransform* transform);
GstFlowReturn dsloggerTransform(GstBaseTransform* transform, GstBuffer* buffer, LogEngine* engine);
