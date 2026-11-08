#include "plugins/fadedrawer/engine/seg_mask.hpp"

SegMask::SegMask(bool showMask)
    : showMask(showMask) {
}

bool SegMask::keep() const {
    return showMask;
}

void SegMask::clearUnlessShown(NvDsObjectMeta* object) const {
    if (!showMask) {
        clear(object);
    }
}

void SegMask::clear(NvDsObjectMeta* object) const {
    g_free(object->mask_params.data);
    object->mask_params.data = nullptr;
    object->mask_params.size = 0;
    object->mask_params.width = 0;
    object->mask_params.height = 0;
    object->mask_params.threshold = 0.0f;
}

void SegMask::attach(DetFadeEngine& engine, NvDsBatchMeta* batch, NvDsFrameMeta* frame, const DetFadeEngine::CachedObject& cached, const NvOSD_ColorParams& border) const {
    if (showMask && !cached.mask.empty() && cached.maskSize > 0) {
        NvDsObjectMeta* object = nvds_acquire_obj_meta_from_pool(batch);
        if (object != nullptr) {
            object->class_id = cached.classId;
            object->object_id = cached.objectId;
            object->confidence = cached.confidence;
            object->rect_params.left = cached.left;
            object->rect_params.top = cached.top;
            object->rect_params.width = cached.width;
            object->rect_params.height = cached.height;
            object->rect_params.border_width = 0;
            object->rect_params.border_color = border;
            object->rect_params.has_bg_color = 0;
            g_free(object->text_params.display_text);
            object->text_params.display_text = nullptr;
            object->text_params.set_bg_clr = 0;
            engine.restoreMask(object, cached);
            nvds_add_obj_meta_to_frame(frame, object, nullptr);
        }
    }
}
