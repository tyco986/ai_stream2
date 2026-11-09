#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

#include <string>

class Capturer;

void capturerAddPads(GstElementClass* elementClass);
void capturerPrepare(GstBaseTransform* transform);
std::string capturerPipelineName(GstElement* element);
GstFlowReturn capturerTransform(GstBaseTransform* transform, GstBuffer* buffer, Capturer* engine);
