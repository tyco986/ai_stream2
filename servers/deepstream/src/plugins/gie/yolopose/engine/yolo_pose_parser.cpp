#include "plugins/gie/yolopose/engine/yolo_pose_parser.hpp"

#include <algorithm>
#include <iostream>
#include <string>

#include "constants/nvinfer_yolo_pose_const.hpp"

YoloPoseParser::YoloPoseParser(
    const std::vector<NvDsInferLayerInfo>& layers,
    const NvDsInferNetworkInfo& network,
    const NvDsInferParseDetectionParams& params)
    : layers(layers),
      network(network),
      params(params) {
}

const NvDsInferLayerInfo* YoloPoseParser::outputLayer() const {
    const NvDsInferLayerInfo* found = nullptr;
    for (const NvDsInferLayerInfo& layer : layers) {
        const bool named = layer.layerName != nullptr;
        const bool output0 = named && std::string(layer.layerName) == nvinferyolopose::LAYER_OUTPUT0;
        const bool output = named && std::string(layer.layerName) == nvinferyolopose::LAYER_OUTPUT;
        if (found == nullptr && (output0 || output)) {
            found = &layer;
        }
    }
    if (found == nullptr && !layers.empty()) {
        found = &layers[0];
    }
    return found;
}

bool YoloPoseParser::readShape(const NvDsInferLayerInfo& layer, unsigned int& numDetections, unsigned int& columns, unsigned int& keypointOffset, unsigned int& numKeypoints) const {
    const NvDsInferDims& dims = layer.inferDims;
    numDetections = 0;
    columns = 0;
    keypointOffset = 0;
    numKeypoints = 0;
    if (dims.numDims == 2) {
        numDetections = static_cast<unsigned int>(dims.d[0]);
        columns = static_cast<unsigned int>(dims.d[1]);
    } else if (dims.numDims == 3 && dims.d[0] == 1) {
        numDetections = static_cast<unsigned int>(dims.d[1]);
        columns = static_cast<unsigned int>(dims.d[2]);
    }
    const bool withClass = columns >= nvinferyolopose::COLUMNS_WITH_CLASS + nvinferyolopose::KEYPOINT_STRIDE && (columns - nvinferyolopose::COLUMNS_WITH_CLASS) % nvinferyolopose::KEYPOINT_STRIDE == 0;
    const bool withoutClass = columns >= nvinferyolopose::COLUMNS_WITHOUT_CLASS + nvinferyolopose::KEYPOINT_STRIDE && (columns - nvinferyolopose::COLUMNS_WITHOUT_CLASS) % nvinferyolopose::KEYPOINT_STRIDE == 0;
    if (withClass) {
        numKeypoints = (columns - nvinferyolopose::COLUMNS_WITH_CLASS) / nvinferyolopose::KEYPOINT_STRIDE;
        keypointOffset = nvinferyolopose::COLUMNS_WITH_CLASS;
    } else if (withoutClass) {
        numKeypoints = (columns - nvinferyolopose::COLUMNS_WITHOUT_CLASS) / nvinferyolopose::KEYPOINT_STRIDE;
        keypointOffset = nvinferyolopose::COLUMNS_WITHOUT_CLASS;
    }
    return numDetections > 0 && numKeypoints >= 1;
}

float YoloPoseParser::clip(float value, float limit) const {
    return std::clamp(value, 0.0f, limit);
}

bool YoloPoseParser::parse(std::vector<NvDsInferInstanceMaskInfo>& objects) const {
    const NvDsInferLayerInfo* layer = layers.empty() ? nullptr : outputLayer();
    unsigned int numDetections = 0;
    unsigned int columns = 0;
    unsigned int keypointOffset = 0;
    unsigned int numKeypoints = 0;
    const bool shaped = layer != nullptr && layer->buffer != nullptr && readShape(*layer, numDetections, columns, keypointOffset, numKeypoints);
    if (!shaped) {
        std::cerr << "NvInferYoloPose: expected [N,5+3K] or [N,6+3K] pose tensor" << std::endl;
    }
    const float* data = shaped ? static_cast<const float*>(layer->buffer) : nullptr;
    const unsigned int numClasses = params.perClassPreclusterThreshold.size();
    for (unsigned int index = 0; shaped && index < numDetections; index++) {
        const float* detection = data + static_cast<size_t>(index) * columns;
        const float confidence = detection[4];
        const int classId = keypointOffset == nvinferyolopose::COLUMNS_WITH_CLASS ? static_cast<int>(detection[5]) : 0;
        const bool knownClass = classId >= 0 && (numClasses == 0 || static_cast<unsigned int>(classId) < numClasses);
        const bool confident = knownClass && numClasses > 0 && confidence >= params.perClassPreclusterThreshold[classId];
        NvDsInferInstanceMaskInfo object{};
        if (confident) {
            object.classId = classId;
            object.detectionConfidence = confidence;
            object.left = clip(detection[0], static_cast<float>(network.width) - 1.0f);
            object.top = clip(detection[1], static_cast<float>(network.height) - 1.0f);
            object.width = clip(detection[2] - detection[0], static_cast<float>(network.width) - 1.0f);
            object.height = clip(detection[3] - detection[1], static_cast<float>(network.height) - 1.0f);
        }
        const bool sized = confident && object.width >= 1.0f && object.height >= 1.0f;
        if (sized) {
            object.mask_width = nvinferyolopose::KEYPOINT_STRIDE;
            object.mask_height = numKeypoints;
            object.mask_size = sizeof(float) * numKeypoints * nvinferyolopose::KEYPOINT_STRIDE;
            object.mask = new float[numKeypoints * nvinferyolopose::KEYPOINT_STRIDE];
            for (unsigned int keypoint = 0; keypoint < numKeypoints; keypoint++) {
                const float x = detection[keypointOffset + keypoint * 3 + 0];
                const float y = detection[keypointOffset + keypoint * 3 + 1];
                const float score = detection[keypointOffset + keypoint * 3 + 2];
                object.mask[keypoint * 3 + 0] = (x - object.left) / object.width;
                object.mask[keypoint * 3 + 1] = (y - object.top) / object.height;
                object.mask[keypoint * 3 + 2] = score;
            }
            objects.push_back(object);
        }
    }
    return shaped;
}
