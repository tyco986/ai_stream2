#pragma once

#include <gst/base/gstbasetransform.h>

class CollectorEngine;

void collectorAddPads(GstElementClass* elementClass);
void collectorConfigure(GObjectClass* objectClass, GstBaseTransformClass* transformClass);
void collectorPrepare(GstBaseTransform* transform);
