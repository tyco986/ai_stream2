#include "plugins/fadedrawer/engine/stgcnpp_fade_engine_with_tracker.hpp"

#include <cstdio>

StgcnppFadeEngineWithTracker::StgcnppFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold, bool showSnap)
    : DetFadeEngine(interval, fadeTime, showLabel),
      PoseFadeEngineWithTracker(interval, fadeTime, showLabel, showPose, poseThreshold, showSnap),
      actions() {
}

const StgcnppFadeEngineWithTracker::ActionMeta* StgcnppFadeEngineWithTracker::actionMeta(NvDsObjectMeta* object) {
    const ActionMeta* action = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr && action == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == fadedrawer::STGCNPP_ACTION_USER_META) {
                action = static_cast<const ActionMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return action;
}

const StgcnppFadeEngineWithTracker::ReadyMeta* StgcnppFadeEngineWithTracker::readyMeta(NvDsObjectMeta* object) {
    const ReadyMeta* ready = nullptr;
    if (object != nullptr) {
        for (NvDsMetaList* item = object->obj_user_meta_list; item != nullptr && ready == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == fadedrawer::STGCNPP_READY_USER_META) {
                ready = static_cast<const ReadyMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return ready;
}

void StgcnppFadeEngineWithTracker::cacheLiveActions(NvDsFrameMeta* frame) {
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
            auto* object = static_cast<NvDsObjectMeta*>(item->data);
            const ActionMeta* action = actionMeta(object);
            if (object != nullptr && action != nullptr && action->label[0] != '\0') {
                LastAction& cached = actions[object->object_id];
                cached.name = action->label;
                cached.confidence = action->conf;
            }
        }
    }
}

void StgcnppFadeEngineWithTracker::writeLabel(NvDsObjectMeta* object) const {
    NvOSD_TextParams* text = &object->text_params;
    g_free(text->display_text);
    text->display_text = nullptr;
    text->set_bg_clr = 0;
    if (showLabel) {
        const ActionMeta* action = actionMeta(object);
        const ReadyMeta* ready = readyMeta(object);
        const auto cached = actions.find(object->object_id);
        const bool live = action != nullptr && action->label[0] != '\0';
        const bool remembered = !live && cached != actions.end();
        const char* name = live ? action->label : (remembered ? cached->second.name.c_str() : "");
        const float actionConf = live ? action->conf : (remembered ? cached->second.confidence : fadedrawer::STGCNPP_ACTION_CONF_NONE);
        const int clipLength = ready != nullptr ? ready->length : 0;
        const long long trackId = object->object_id == UNTRACKED_OBJECT_ID ? -1LL : static_cast<long long>(object->object_id);
        char line[fadedrawer::LABEL_LINE_SIZE];
        std::snprintf(
            line,
            sizeof(line),
            "%s%c%.2f%c%d%c%.2f%c%lld",
            name,
            fadedrawer::LABEL_SEP,
            actionConf,
            fadedrawer::LABEL_SEP,
            clipLength,
            fadedrawer::LABEL_SEP,
            object->confidence,
            fadedrawer::LABEL_SEP,
            trackId);
        text->display_text = g_strdup(line);
        text->x_offset = static_cast<unsigned int>(object->rect_params.left);
        const int y = static_cast<int>(object->rect_params.top) - fadedrawer::LABEL_Y_OFFSET;
        text->y_offset = y < 0 ? 0u : static_cast<unsigned int>(y);
        text->font_params.font_name = const_cast<char*>(fadedrawer::FONT_NAME);
        text->font_params.font_size = fadedrawer::FONT_SIZE;
        text->set_bg_clr = 1;
    }
}

void StgcnppFadeEngineWithTracker::processFrame(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    cacheLiveActions(frame);
    DetFadeEngineWithTracker::processFrame(batch, frame);
}
