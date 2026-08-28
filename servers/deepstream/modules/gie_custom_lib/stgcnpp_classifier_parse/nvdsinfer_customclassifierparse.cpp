#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "nvdsinfer_custom_impl.h"

namespace {

bool parse_stgcnpp_classifier(
    std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
    float classifierThreshold,
    std::vector<NvDsInferAttribute> &attrList,
    std::string &descString)
{
    bool ok = false;
    if (!outputLayersInfo.empty()) {
        NvDsInferLayerInfo const &layer = outputLayersInfo[0];
        float *data = static_cast<float *>(layer.buffer);
        unsigned int numClasses = layer.inferDims.numElements;
        if (data != nullptr && numClasses > 0) {
            unsigned int best = 0;
            float bestScore = data[0];
            for (unsigned int index = 1; index < numClasses; index++) {
                if (data[index] > bestScore) {
                    bestScore = data[index];
                    best = index;
                }
            }
            if (bestScore >= classifierThreshold) {
                NvDsInferAttribute attr{};
                char buf[64];
                attr.attributeIndex = 0;
                attr.attributeValue = best;
                attr.attributeConfidence = bestScore;
                std::snprintf(buf, sizeof(buf), "%u|%.4f", best, bestScore);
                attr.attributeLabel = strdup(buf);
                attrList.push_back(attr);
                descString.assign(buf);
            }
            ok = true;
        }
    }
    return ok;
}

}  // namespace

extern "C" bool NvDsInferClassiferParseCustomStgcnpp(
    std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
    NvDsInferNetworkInfo const &networkInfo,
    float classifierThreshold,
    std::vector<NvDsInferAttribute> &attrList,
    std::string &descString)
{
    (void)networkInfo;
    bool ok = parse_stgcnpp_classifier(
        outputLayersInfo, classifierThreshold, attrList, descString);
    return ok;
}

