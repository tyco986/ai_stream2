#include "plugins/fadedrawer/engine/det_fade_engine_with_tracker.hpp"

#include <cstdio>
#include <cstring>

#include "constants/boxsnapshot_const.hpp"

DetFadeEngineWithTracker::DetFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showSnap)
    : DetFadeEngine(interval, fadeTime, showLabel),
      tracks(),
      showSnap(showSnap) {
}

void DetFadeEngineWithTracker::dropStream(int padIndex) {
    tracks.erase(padIndex);
    DetFadeEngine::dropStream(padIndex);
}

void DetFadeEngineWithTracker::writeLabel(NvDsObjectMeta* object) const {
    NvOSD_TextParams* text = &object->text_params;
    g_free(text->display_text);
    text->display_text = nullptr;
    text->set_bg_clr = 0;
    if (showLabel && object->object_id != UNTRACKED_OBJECT_ID) {
        char line[fadedrawer::LABEL_LINE_SIZE];
        std::snprintf(line, sizeof(line), "%s%c%.2f%c%llu", object->obj_label, fadedrawer::LABEL_SEP, object->confidence, fadedrawer::LABEL_SEP, static_cast<unsigned long long>(object->object_id));
        text->display_text = g_strdup(line);
        text->x_offset = static_cast<unsigned int>(object->rect_params.left);
        const int y = static_cast<int>(object->rect_params.top) - fadedrawer::LABEL_Y_OFFSET;
        text->y_offset = y < 0 ? 0u : static_cast<unsigned int>(y);
        text->font_params.font_name = const_cast<char*>(fadedrawer::FONT_NAME);
        text->font_params.font_size = fadedrawer::FONT_SIZE;
        text->set_bg_clr = 1;
    }
}

const BoxSnapshotMeta* DetFadeEngineWithTracker::findSnapshot(NvDsFrameMeta* frame) const {
    const BoxSnapshotMeta* snapshot = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && snapshot == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == boxsnapshot::USER_META) {
                snapshot = static_cast<const BoxSnapshotMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return snapshot;
}

void DetFadeEngineWithTracker::cacheLiveTracks(NvDsFrameMeta* frame) {
    std::unordered_map<std::uint64_t, TrackCache>& streamTracks = tracks[static_cast<int>(frame->pad_index)];
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr && object->object_id != UNTRACKED_OBJECT_ID) {
            TrackCache& track = streamTracks[object->object_id];
            if (object->confidence >= fadedrawer::MIN_CACHED_CONFIDENCE) {
                track.confidence = object->confidence;
            }
        }
    }
}

void DetFadeEngineWithTracker::applyTrackCache(NvDsObjectMeta* object, int padIndex) const {
    const auto stream = tracks.find(padIndex);
    const bool tracked = object->object_id != UNTRACKED_OBJECT_ID && stream != tracks.end();
    if (tracked) {
        const auto track = stream->second.find(object->object_id);
        if (track != stream->second.end()) {
            object->confidence = track->second.confidence;
        }
    }
}

void DetFadeEngineWithTracker::renewSnapshot(StreamState& state, const BoxSnapshotMeta* snapshot) const {
    state.objects.clear();
    if (snapshot != nullptr && snapshot->boxes != nullptr) {
        for (guint index = 0; index < snapshot->num_boxes; index++) {
            const BoxSnapshotBox& box = snapshot->boxes[index];
            CachedObject cached;
            cached.left = box.left;
            cached.top = box.top;
            cached.width = box.width;
            cached.height = box.height;
            cached.confidence = box.confidence;
            cached.classId = box.class_id;
            cached.objectId = box.object_id;
            cached.label = box.label;
            if (snapshot->masks != nullptr && box.mask_size > 0) {
                NvOSD_MaskParams mask{};
                mask.data = snapshot->masks + box.mask_offset / sizeof(float);
                mask.size = box.mask_size;
                mask.width = box.mask_width;
                mask.height = box.mask_height;
                mask.threshold = box.mask_threshold;
                cacheMask(cached, mask);
            }
            state.objects.push_back(std::move(cached));
        }
    }
}

void DetFadeEngineWithTracker::drawSnapshot(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const StreamState& state, const NvOSD_ColorParams& border, float alpha) {
    std::size_t index = 0;
    while (index < state.objects.size()) {
        NvDsDisplayMeta* display = nvds_acquire_display_meta_from_pool(batch);
        display->num_rects = 0;
        while (index < state.objects.size() && display->num_rects < MAX_ELEMENTS_IN_DISPLAY_META) {
            const CachedObject& cached = state.objects[index];
            NvOSD_RectParams* rect = &display->rect_params[display->num_rects];
            rect->left = cached.left;
            rect->top = cached.top;
            rect->width = cached.width;
            rect->height = cached.height;
            rect->border_color = border;
            rect->border_width = fadedrawer::BOX_WIDTH;
            rect->has_bg_color = 0;
            display->num_rects += 1;
            index += 1;
        }
        nvds_add_display_meta_to_frame(frame, display);
    }
    for (const CachedObject& cached : state.objects) {
        decorateCached(batch, frame, cached, alpha);
    }
}

void DetFadeEngineWithTracker::drawLive(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    const NvOSD_ColorParams border = colorWithAlpha(fadedrawer::COLOR_PURPLE, 1.0f);
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            applyTrackCache(object, static_cast<int>(frame->pad_index));
            paint(batch, frame, object, border, 1.0f);
        }
    }
}

void DetFadeEngineWithTracker::processFrame(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    StreamState& state = streams[static_cast<int>(frame->pad_index)];
    cacheLiveTracks(frame);
    const BoxSnapshotMeta* snapshot = showSnap ? findSnapshot(frame) : nullptr;
    if (showSnap && snapshot != nullptr) {
        renewSnapshot(state, snapshot);
    }
    drawLive(batch, frame);
    if (showSnap) {
        const int lutSize = static_cast<int>(alphaLut.size());
        const bool snapped = snapshot != nullptr;
        const int phase = snapped ? 0 : state.phase;
        const float alpha = alphaLut[static_cast<std::size_t>(phase % lutSize)];
        drawSnapshot(batch, frame, state, boxColor(frame, alpha), alpha);
        state.phase = snapped ? (lutSize > 0 ? 1 % lutSize : 0) : (phase + 1) % lutSize;
    }
}
