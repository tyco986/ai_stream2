#include "plugins/fadedrawer/seg_fade_engine.hpp"

SegFadeEngine::SegFadeEngine(int interval, int fadeTime, bool showLabel, bool showMask)
    : DetFadeEngine(interval, fadeTime, showLabel),
      showMask(showMask) {
}

bool SegFadeEngine::keepMask() const {
    return showMask;
}

void SegFadeEngine::clearMask(NvDsObjectMeta* object) const {
    g_free(object->mask_params.data);
    object->mask_params.data = nullptr;
    object->mask_params.size = 0;
    object->mask_params.width = 0;
    object->mask_params.height = 0;
    object->mask_params.threshold = 0.0f;
}

void SegFadeEngine::decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) {
    (void)alpha;
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
            object->rect_params.has_bg_color = 0;
            g_free(object->text_params.display_text);
            object->text_params.display_text = nullptr;
            object->text_params.set_bg_clr = 0;
            restoreMask(object, cached);
            nvds_add_obj_meta_to_frame(frame, object, nullptr);
        }
    }
}

void SegFadeEngine::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)alpha;
    if (!showMask) {
        clearMask(object);
    }
}
