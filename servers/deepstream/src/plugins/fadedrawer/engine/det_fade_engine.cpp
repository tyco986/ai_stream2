#include "plugins/fadedrawer/engine/det_fade_engine.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

NvOSD_ColorParams DetFadeEngine::colorWithAlpha(const float rgb[4], float alpha) {
    NvOSD_ColorParams color;
    color.red = rgb[0];
    color.green = rgb[1];
    color.blue = rgb[2];
    color.alpha = alpha;
    return color;
}

const DetFadeEngine::PresenceEventMeta* DetFadeEngine::presenceEvent(NvDsFrameMeta* frame) {
    const PresenceEventMeta* event = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && event == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == fadedrawer::PRESENCE_EVENT_USER_META) {
                event = static_cast<const PresenceEventMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return event;
}

const float* DetFadeEngine::borderRgb(NvDsFrameMeta* frame) {
    const float* rgb = fadedrawer::COLOR_GREEN;
    const PresenceEventMeta* event = presenceEvent(frame);
    if (event != nullptr) {
        bool hasAlert = false;
        bool hasTransit = false;
        unsigned int count = event->numClasses;
        if (count > fadedrawer::PRESENCE_EVENT_CODE_LEN) {
            count = fadedrawer::PRESENCE_EVENT_CODE_LEN;
        }
        for (unsigned int index = 0; index < count; index++) {
            const char code = event->eventCodes[index];
            hasAlert = hasAlert || code == '1';
            hasTransit = hasTransit || code == '2';
        }
        if (hasAlert) {
            rgb = fadedrawer::COLOR_RED;
        } else if (hasTransit) {
            rgb = fadedrawer::COLOR_YELLOW;
        }
    }
    return rgb;
}

DetFadeEngine::DetFadeEngine(int interval, int fadeTime, bool showLabel)
    : streams(),
      alphaLut(buildLut(interval, fadeTime)),
      showLabel(showLabel),
      interval(interval),
      fadeTime(fadeTime) {
}

void DetFadeEngine::dropStream(int padIndex) {
    streams.erase(padIndex);
}

std::vector<float> DetFadeEngine::buildLut(int interval, int fadeTime) const {
    std::vector<float> lut;
    if (fadeTime <= 0) {
        lut.push_back(1.0f);
    } else {
        const int period = interval > 0 ? interval : 1;
        const int mid = period / 2;
        const int tail = period - mid;
        std::vector<float> triangle;
        triangle.reserve(static_cast<std::size_t>(period));
        for (int index = 0; index < period; index++) {
            const float rise = mid > 0 ? 1.0f - static_cast<float>(index) / static_cast<float>(mid) : 1.0f;
            const float fall = tail > 0 ? static_cast<float>(index - mid) / static_cast<float>(tail) : 0.0f;
            const float progress = index <= mid ? rise : fall;
            const float alpha = fadedrawer::MIN_ALPHA + (1.0f - fadedrawer::MIN_ALPHA) * progress;
            triangle.push_back(std::round(alpha * 100.0f) / 100.0f);
        }
        for (int repeat = 0; repeat < fadeTime; repeat++) {
            const auto begin = repeat == 0 ? triangle.begin() : triangle.begin() + 1;
            if (begin != triangle.end()) {
                lut.insert(lut.end(), begin, triangle.end());
            }
        }
    }
    if (lut.empty()) {
        lut.push_back(1.0f);
    }
    return lut;
}

NvOSD_ColorParams DetFadeEngine::boxColor(NvDsFrameMeta* frame, float alpha) const {
    return colorWithAlpha(borderRgb(frame), alpha);
}

void DetFadeEngine::writeLabel(NvDsObjectMeta* object) const {
    NvOSD_TextParams* text = &object->text_params;
    g_free(text->display_text);
    text->display_text = nullptr;
    text->set_bg_clr = 0;
    if (showLabel) {
        char line[fadedrawer::LABEL_LINE_SIZE];
        std::snprintf(line, sizeof(line), "%s%c%.2f", object->obj_label, fadedrawer::LABEL_SEP, object->confidence);
        text->display_text = g_strdup(line);
        text->x_offset = static_cast<unsigned int>(object->rect_params.left);
        const int y = static_cast<int>(object->rect_params.top) - fadedrawer::LABEL_Y_OFFSET;
        text->y_offset = y < 0 ? 0u : static_cast<unsigned int>(y);
        text->font_params.font_name = const_cast<char*>(fadedrawer::FONT_NAME);
        text->font_params.font_size = fadedrawer::FONT_SIZE;
        text->set_bg_clr = 1;
    }
}

bool DetFadeEngine::keepMask() const {
    return false;
}

void DetFadeEngine::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)object;
    (void)alpha;
}

void DetFadeEngine::decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) {
    (void)batch;
    (void)frame;
    (void)cached;
    (void)alpha;
}

void DetFadeEngine::paint(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, const NvOSD_ColorParams& border, float alpha) {
    object->rect_params.border_color = border;
    object->rect_params.border_width = fadedrawer::BOX_WIDTH;
    object->rect_params.has_bg_color = 0;
    writeLabel(object);
    if (showLabel) {
        object->text_params.font_params.font_color = colorWithAlpha(fadedrawer::COLOR_TEXT, alpha);
        object->text_params.text_bg_clr = colorWithAlpha(fadedrawer::COLOR_TEXT_BG, alpha);
    }
    decorateObject(batch, frame, object, alpha);
}

void DetFadeEngine::cacheMask(CachedObject& cached, const NvOSD_MaskParams& mask) const {
    cached.mask.clear();
    cached.maskSize = 0;
    cached.maskWidth = 0;
    cached.maskHeight = 0;
    cached.maskThreshold = 0.0f;
    if (mask.data != nullptr && mask.size > 0) {
        cached.maskSize = mask.size;
        cached.maskWidth = mask.width;
        cached.maskHeight = mask.height;
        cached.maskThreshold = mask.threshold;
        cached.mask.resize(mask.size / sizeof(float));
        std::memcpy(cached.mask.data(), mask.data, mask.size);
    }
}

void DetFadeEngine::restoreMask(NvDsObjectMeta* object, const CachedObject& cached) const {
    if (!cached.mask.empty() && cached.maskSize > 0) {
        object->mask_params.width = cached.maskWidth;
        object->mask_params.height = cached.maskHeight;
        object->mask_params.size = cached.maskSize;
        object->mask_params.threshold = cached.maskThreshold;
        object->mask_params.data = static_cast<float*>(g_malloc(cached.maskSize));
        std::memcpy(object->mask_params.data, cached.mask.data(), cached.maskSize);
    }
}

void DetFadeEngine::renewCache(StreamState& state, NvDsFrameMeta* frame) const {
    state.objects.clear();
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            CachedObject cached;
            cached.left = object->rect_params.left;
            cached.top = object->rect_params.top;
            cached.width = object->rect_params.width;
            cached.height = object->rect_params.height;
            cached.confidence = object->confidence;
            cached.classId = object->class_id;
            cached.objectId = object->object_id;
            cached.label = object->obj_label;
            if (keepMask()) {
                cacheMask(cached, object->mask_params);
            }
            state.objects.push_back(std::move(cached));
        }
    }
}

void DetFadeEngine::replay(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const StreamState& state, const NvOSD_ColorParams& border, float alpha) {
    for (const CachedObject& cached : state.objects) {
        NvDsObjectMeta* object = nvds_acquire_obj_meta_from_pool(batch);
        if (object != nullptr) {
            object->class_id = cached.classId;
            object->object_id = cached.objectId;
            object->confidence = cached.confidence;
            object->rect_params.left = cached.left;
            object->rect_params.top = cached.top;
            object->rect_params.width = cached.width;
            object->rect_params.height = cached.height;
            g_strlcpy(object->obj_label, cached.label.c_str(), sizeof(object->obj_label));
            restoreMask(object, cached);
            nvds_add_obj_meta_to_frame(frame, object, nullptr);
            paint(batch, frame, object, border, alpha);
        }
    }
}

void DetFadeEngine::processFrame(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    StreamState& state = streams[static_cast<int>(frame->pad_index)];
    const bool inferred = frame->bInferDone;
    if (inferred) {
        renewCache(state, frame);
    }
    const int lutSize = static_cast<int>(alphaLut.size());
    const int phase = inferred ? 0 : state.phase;
    const float alpha = alphaLut[static_cast<std::size_t>(phase % lutSize)];
    const NvOSD_ColorParams border = boxColor(frame, alpha);
    if (inferred) {
        for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
            auto* object = static_cast<NvDsObjectMeta*>(item->data);
            if (object != nullptr) {
                paint(batch, frame, object, border, alpha);
            }
        }
    } else {
        replay(batch, frame, state, border, alpha);
    }
    state.phase = inferred ? (lutSize > 0 ? 1 % lutSize : 0) : (phase + 1) % lutSize;
}
