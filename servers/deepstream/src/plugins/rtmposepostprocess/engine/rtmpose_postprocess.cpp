#include "plugins/rtmposepostprocess/engine/rtmpose_postprocess.hpp"

#include <string>

#include <cuda_runtime_api.h>
#include "nvdsinfer.h"

#include "constants/boxsnapshot_const.hpp"
#include "constants/rtmposepostprocess_const.hpp"
#include "plugins/boxsnapshot/engine/boxsnapshot.hpp"

RtmposePostprocess::RtmposePostprocess(int sgieUniqueId, int inferWidth, int inferHeight, float padding)
    : expand(inferWidth, inferHeight, padding) {
    uniqueId = sgieUniqueId;
}

void RtmposePostprocess::setSgieUniqueId(int sgieUniqueId) {
    uniqueId = sgieUniqueId;
}

int RtmposePostprocess::sgieUniqueId() const {
    return uniqueId;
}

void RtmposePostprocess::setGeometry(int inferWidth, int inferHeight, float padding) {
    expand.inferWidth = inferWidth;
    expand.inferHeight = inferHeight;
    expand.padding = padding;
}

int RtmposePostprocess::inferWidth() const {
    return expand.inferWidth;
}

int RtmposePostprocess::inferHeight() const {
    return expand.inferHeight;
}

float RtmposePostprocess::padding() const {
    return expand.padding;
}

bool RtmposePostprocess::batchTargetsSgie(const GstNvDsPreProcessBatchMeta* preprocessBatch) const {
    bool hit = false;
    if (preprocessBatch != nullptr) {
        for (guint64 id : preprocessBatch->target_unique_ids) {
            hit = hit || static_cast<int>(id) == uniqueId;
        }
    }
    return hit;
}

const NvDsInferTensorMeta* RtmposePostprocess::tensorFromRoi(NvDsRoiMeta* roi) const {
    const NvDsInferTensorMeta* tensor = nullptr;
    if (roi != nullptr) {
        for (NvDsMetaList* item = roi->roi_user_meta_list; item != nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == NVDSINFER_TENSOR_OUTPUT_META) {
                tensor = static_cast<const NvDsInferTensorMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return tensor;
}

RtmposePostprocess::Keypoints RtmposePostprocess::keypointsFromTensor(const NvDsInferTensorMeta* tensorMeta) {
    const float* data = nullptr;
    int joints = 0;
    if (tensorMeta != nullptr && static_cast<int>(tensorMeta->unique_id) == uniqueId) {
        for (guint i = 0; i < tensorMeta->num_output_layers; ++i) {
            const NvDsInferLayerInfo& layer = tensorMeta->output_layers_info[i];
            const bool named = layer.layerName != nullptr && std::string(layer.layerName) == rtmposepostprocess::KEYPOINTS_LAYER;
            const int elements = named ? static_cast<int>(layer.inferDims.numElements) : 0;
            if (elements >= rtmposepostprocess::MIN_KEYPOINT_ELEMENTS) {
                data = static_cast<const float*>(tensorMeta->out_buf_ptrs_host[i]);
                if (data == nullptr && tensorMeta->out_buf_ptrs_dev[i] != nullptr) {
                    hostScratch.resize(static_cast<size_t>(elements));
                    cudaMemcpy(
                        hostScratch.data(),
                        tensorMeta->out_buf_ptrs_dev[i],
                        static_cast<size_t>(elements) * sizeof(float),
                        cudaMemcpyDeviceToHost);
                    data = hostScratch.data();
                }
                joints = elements / 3;
            }
        }
    }
    Keypoints keypoints{data, joints};
    return keypoints;
}

void RtmposePostprocess::writeMaskParams(
    NvDsObjectMeta* object, const CropGeom& crop, const float* cropXyScore, int numJoints) const {
    const float boxWidth = object->rect_params.width;
    const float boxHeight = object->rect_params.height;
    const float ratioX = crop.destWidth / static_cast<float>(crop.srcWidth);
    const float ratioY = crop.destHeight / static_cast<float>(crop.srcHeight);
    g_free(object->mask_params.data);
    object->mask_params.data = nullptr;
    object->mask_params.size = 0;
    object->mask_params.width = 0;
    object->mask_params.height = 0;
    const bool usable = numJoints >= 1 && boxWidth >= 1.0f && boxHeight >= 1.0f && ratioX > 0.0f && ratioY > 0.0f;
    if (usable) {
        object->mask_params.width = 3;
        object->mask_params.height = static_cast<guint>(numJoints);
        object->mask_params.size = sizeof(float) * static_cast<guint>(numJoints) * 3;
        object->mask_params.data = static_cast<gfloat*>(g_malloc(object->mask_params.size));
        for (int joint = 0; joint < numJoints; ++joint) {
            const float frameX = static_cast<float>(crop.srcLeft) +
                (cropXyScore[joint * 3 + 0] - static_cast<float>(crop.offsetLeft)) / ratioX;
            const float frameY = static_cast<float>(crop.srcTop) +
                (cropXyScore[joint * 3 + 1] - static_cast<float>(crop.offsetTop)) / ratioY;
            object->mask_params.data[joint * 3 + 0] = (frameX - object->rect_params.left) / boxWidth;
            object->mask_params.data[joint * 3 + 1] = (frameY - object->rect_params.top) / boxHeight;
            object->mask_params.data[joint * 3 + 2] = cropXyScore[joint * 3 + 2];
        }
    }
}

void RtmposePostprocess::mirrorSnapshot(NvDsFrameMeta* frame, NvDsObjectMeta* object) const {
    BoxSnapshotMeta* snapshot = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && snapshot == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == boxsnapshot::USER_META) {
                snapshot = static_cast<BoxSnapshotMeta*>(userMeta->user_meta_data);
            }
        }
    }
    const gint64 key = object != nullptr ? object->misc_obj_info[boxsnapshot::SNAPSHOT_KEY_FIELD] : 0;
    BoxSnapshotBox* box = nullptr;
    if (snapshot != nullptr && snapshot->boxes != nullptr && key != 0) {
        for (guint index = 0; index < snapshot->num_boxes && box == nullptr; index++) {
            if (snapshot->boxes[index].snapshot_key == key) {
                box = &snapshot->boxes[index];
            }
        }
    }
    const bool open = box != nullptr && box->mask_size == 0 && object->mask_params.data != nullptr && object->mask_params.width == 3 &&
        object->mask_params.height >= 1 && object->rect_params.width > 0.0f && object->rect_params.height > 0.0f && box->width > 0.0f && box->height > 0.0f;
    if (open) {
        const guint count = object->mask_params.height;
        const guint bytes = static_cast<guint>(sizeof(float) * count * 3);
        const guint offset = snapshot->mask_bytes;
        auto* grown = static_cast<gfloat*>(g_realloc(snapshot->masks, static_cast<gsize>(offset) + bytes));
        if (grown != nullptr) {
            const float* data = object->mask_params.data;
            const float srcWidth = object->rect_params.width;
            const float srcHeight = object->rect_params.height;
            auto* dest = reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(grown) + offset);
            for (guint joint = 0; joint < count; joint++) {
                const float frameX = object->rect_params.left + data[joint * 3] * srcWidth;
                const float frameY = object->rect_params.top + data[joint * 3 + 1] * srcHeight;
                dest[joint * 3] = (frameX - box->left) / box->width;
                dest[joint * 3 + 1] = (frameY - box->top) / box->height;
                dest[joint * 3 + 2] = data[joint * 3 + 2];
            }
            snapshot->masks = grown;
            snapshot->mask_bytes = offset + bytes;
            box->mask_offset = offset;
            box->mask_size = bytes;
            box->mask_width = 3;
            box->mask_height = count;
            box->mask_threshold = object->mask_params.threshold;
        }
    }
}

void RtmposePostprocess::processRoi(NvDsRoiMeta* roi) {
    NvDsObjectMeta* object = roi != nullptr ? roi->object_meta : nullptr;
    NvDsFrameMeta* frame = roi != nullptr ? roi->frame_meta : nullptr;
    int frameWidth = 0;
    int frameHeight = 0;
    if (frame != nullptr) {
        frameWidth = static_cast<int>(frame->pipeline_width > 0 ? frame->pipeline_width : frame->source_frame_width);
        frameHeight = static_cast<int>(frame->pipeline_height > 0 ? frame->pipeline_height : frame->source_frame_height);
    }
    Keypoints keypoints = keypointsFromTensor(tensorFromRoi(roi));
    const bool ready = object != nullptr && frameWidth > 0 && frameHeight > 0 && keypoints.data != nullptr && keypoints.joints >= 1;
    if (ready) {
        CropGeom crop = expand.compute(
            object->rect_params.left,
            object->rect_params.top,
            object->rect_params.width,
            object->rect_params.height,
            frameWidth,
            frameHeight);
        writeMaskParams(object, crop, keypoints.data, keypoints.joints);
        mirrorSnapshot(frame, object);
    }
}

void RtmposePostprocess::apply(NvDsBatchMeta* batch) {
    if (batch != nullptr) {
        for (NvDsMetaList* item = batch->batch_user_meta_list; item != nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == NVDS_PREPROCESS_BATCH_META) {
                auto* preprocessBatch = static_cast<GstNvDsPreProcessBatchMeta*>(userMeta->user_meta_data);
                if (batchTargetsSgie(preprocessBatch)) {
                    for (NvDsRoiMeta& roi : preprocessBatch->roi_vector) {
                        processRoi(&roi);
                    }
                }
            }
        }
    }
}
