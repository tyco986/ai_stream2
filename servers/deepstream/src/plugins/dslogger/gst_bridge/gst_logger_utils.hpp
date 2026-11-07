#pragma once

#include <gst/base/gstbasetransform.h>

#include <string>

class LogEngine;

void dsloggerAddPads(GstElementClass* elementClass);
void dsloggerPrepare(GstBaseTransform* transform);
std::string dsloggerPipelineName(GstElement* element);
GstFlowReturn dsloggerTransform(GstBaseTransform* transform, GstBuffer* buffer, LogEngine* engine);
