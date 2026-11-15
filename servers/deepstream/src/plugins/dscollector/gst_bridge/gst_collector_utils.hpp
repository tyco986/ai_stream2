#pragma once

#include <gst/base/gstbasetransform.h>

class CollectorEngine;

void collectorAddPads(GstElementClass* elementClass);
void collectorPrepare(GstBaseTransform* transform);
GstFlowReturn collectorTransform(GstBaseTransform* transform, GstBuffer* buffer, CollectorEngine* engine);
