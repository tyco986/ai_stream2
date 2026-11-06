#pragma once

#include <gst/base/gstbasetransform.h>

class DetFadeEngine;

void fadedrawerAddPads(GstElementClass* elementClass);
void fadedrawerPrepare(GstBaseTransform* transform);
GstFlowReturn fadedrawerTransform(GstBaseTransform* transform, GstBuffer* buffer, DetFadeEngine* engine);
