#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "nvdsinfer_custom_impl.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLIP(a, minv, maxv) (MAX(MIN((a), (maxv)), (minv)))

static const NvDsInferLayerInfo *
yolo_find_layer(
    std::vector<NvDsInferLayerInfo> const &layers,
    const char *name)
{
    const NvDsInferLayerInfo *found = nullptr;
    for (const auto &layer : layers) {
        if (layer.layerName && name && std::string(layer.layerName) == name)
            found = &layer;
    }
    return found;
}

extern "C" bool NvDsInferYoloPose(
    std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
    NvDsInferNetworkInfo const &networkInfo,
    NvDsInferParseDetectionParams const &detectionParams,
    std::vector<NvDsInferInstanceMaskInfo> &objectList)
{
    bool ok = !outputLayersInfo.empty();
    const NvDsInferLayerInfo *layer = nullptr;
    unsigned int numDet = 0;
    unsigned int cols = 0;
    unsigned int numKeypoints = 0;
    unsigned int kptOffset = 6;
    const unsigned int numClasses =
        (unsigned int)detectionParams.perClassPreclusterThreshold.size();
    const char *log_enable = std::getenv("ENABLE_DEBUG");

    if (ok) {
        layer = yolo_find_layer(outputLayersInfo, "output0");
        if (!layer)
            layer = yolo_find_layer(outputLayersInfo, "output");
        if (!layer)
            layer = &outputLayersInfo[0];
        ok = layer->buffer != nullptr;
    }
    if (ok) {
        const NvDsInferDims &dims = layer->inferDims;
        if (dims.numDims == 2) {
            numDet = (unsigned int)dims.d[0];
            cols = (unsigned int)dims.d[1];
        } else if (dims.numDims == 3 && dims.d[0] == 1) {
            numDet = (unsigned int)dims.d[1];
            cols = (unsigned int)dims.d[2];
        }
        if (cols >= 9 && (cols - 6) % 3 == 0) {
            numKeypoints = (cols - 6) / 3;
            kptOffset = 6;
        } else if (cols >= 8 && (cols - 5) % 3 == 0) {
            numKeypoints = (cols - 5) / 3;
            kptOffset = 5;
        }
        ok = numDet > 0 && numKeypoints >= 1;
    }
    if (!ok) {
        std::cerr << "NvDsInferYoloPose: expected [N,5+3K] or [N,6+3K] pose tensor"
                  << std::endl;
    }

    const float *data = ok ? (const float *)layer->buffer : nullptr;
    for (unsigned int i = 0; ok && i < numDet; ++i) {
        const float *det = data + (size_t)i * cols;
        const float conf = det[4];
        int classId = 0;
        NvDsInferInstanceMaskInfo object{};
        float boxW = 0.0f;
        float boxH = 0.0f;
        bool keep = true;

        if (kptOffset == 6)
            classId = (int)det[5];
        if (classId < 0 || (numClasses > 0 && (unsigned int)classId >= numClasses))
            keep = false;
        if (keep && conf < detectionParams.perClassPreclusterThreshold[classId])
            keep = false;

        if (keep) {
            object.classId = classId;
            object.detectionConfidence = conf;
            object.left = CLIP(det[0], 0, networkInfo.width - 1);
            object.top = CLIP(det[1], 0, networkInfo.height - 1);
            object.width = CLIP(det[2] - det[0], 0, networkInfo.width - 1);
            object.height = CLIP(det[3] - det[1], 0, networkInfo.height - 1);
            boxW = object.width;
            boxH = object.height;
            keep = boxW >= 1.0f && boxH >= 1.0f;
        }
        if (keep) {
            object.mask_width = 3;
            object.mask_height = numKeypoints;
            object.mask_size = sizeof(float) * numKeypoints * 3;
            object.mask = new float[numKeypoints * 3];
            for (unsigned int k = 0; k < numKeypoints; k++) {
                const float kx = det[kptOffset + k * 3 + 0];
                const float ky = det[kptOffset + k * 3 + 1];
                const float ks = det[kptOffset + k * 3 + 2];
                object.mask[k * 3 + 0] = (kx - object.left) / boxW;
                object.mask[k * 3 + 1] = (ky - object.top) / boxH;
                object.mask[k * 3 + 2] = ks;
            }
            if (log_enable != nullptr && std::stoi(log_enable)) {
                std::cout << "pose label/conf/ x/y w/h K -- "
                          << classId << " " << conf << " "
                          << object.left << " " << object.top << " "
                          << object.width << " " << object.height << " "
                          << numKeypoints << std::endl;
            }
            objectList.push_back(object);
        }
    }
    return ok;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
CHECK_CUSTOM_INSTANCE_MASK_PARSE_FUNC_PROTOTYPE(NvDsInferYoloPose);
#pragma GCC diagnostic pop
