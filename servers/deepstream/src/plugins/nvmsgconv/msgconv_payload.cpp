#include "plugins/nvmsgconv/msgconv_payload.hpp"

#include <cstring>

#include "nvmsgconv.h"
#include "plugins/dscollector/engine/collector_line.hpp"

#if defined(MSGCONV_PRESENCE)
#include "constants/event_meta_const.hpp"
#include "constants/presenceeventcoder_const.hpp"
#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"
#endif

bool MsgconvPayload::publishable(NvDsFrameMeta* frame) const {
    const bool collected = frame != nullptr && collectorLine(frame) != nullptr;
#if defined(MSGCONV_PRESENCE)
    const PresenceEventMeta* event = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && event == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == presenceeventcoder::USER_META) {
                event = static_cast<const PresenceEventMeta*>(userMeta->user_meta_data);
            }
        }
    }
    unsigned count = event != nullptr ? event->event.slotCount : 0;
    count = count > static_cast<unsigned>(eventmeta::SLOT_COUNT) ? static_cast<unsigned>(eventmeta::SLOT_COUNT) : count;
    bool active = false;
    for (unsigned index = 0; index < count && !active; index++) {
        active = event->event.slots[index].code != eventmeta::ABSENT;
    }
    const bool publish = collected && active;
#else
    const bool publish = collected;
#endif
    return publish;
}

std::string MsgconvPayload::line(NvDsFrameMeta* frame) const {
    const char* collected = collectorLine(frame);
    return collected != nullptr ? std::string(collected) : std::string();
}

extern "C" NvDsMsg2pCtx* nvds_msg2p_ctx_create(const gchar*, NvDsPayloadType type) {
    auto* ctx = static_cast<NvDsMsg2pCtx*>(g_malloc0(sizeof(NvDsMsg2pCtx)));
    ctx->payloadType = type;
    ctx->privData = new MsgconvPayload();
    return ctx;
}

extern "C" void nvds_msg2p_ctx_destroy(NvDsMsg2pCtx* ctx) {
    if (ctx != nullptr) {
        delete static_cast<MsgconvPayload*>(ctx->privData);
        g_free(ctx);
    }
}

extern "C" NvDsPayload* nvds_msg2p_generate(NvDsMsg2pCtx*, NvDsEvent*, guint) {
    return nullptr;
}

extern "C" NvDsPayload** nvds_msg2p_generate_multiple(NvDsMsg2pCtx*, NvDsEvent*, guint, guint* payloadCount) {
    if (payloadCount != nullptr) {
        *payloadCount = 0;
    }
    return nullptr;
}

extern "C" NvDsPayload* nvds_msg2p_generate_new(NvDsMsg2pCtx* ctx, void* metadataInfo) {
    NvDsPayload* payload = nullptr;
    auto* info = static_cast<NvDsMsg2pMetaInfo*>(metadataInfo);
    auto* state = ctx != nullptr ? static_cast<MsgconvPayload*>(ctx->privData) : nullptr;
    auto* frame = info != nullptr ? static_cast<NvDsFrameMeta*>(info->frameMeta) : nullptr;
    const std::string line = state != nullptr && frame != nullptr && state->publishable(frame) ? state->line(frame) : std::string();
    if (!line.empty()) {
        payload = static_cast<NvDsPayload*>(g_malloc0(sizeof(NvDsPayload)));
        payload->payload = g_malloc(line.size());
        std::memcpy(payload->payload, line.data(), line.size());
        payload->payloadSize = static_cast<guint>(line.size());
    }
    return payload;
}

extern "C" NvDsPayload** nvds_msg2p_generate_multiple_new(NvDsMsg2pCtx*, void*, guint* payloadCount) {
    if (payloadCount != nullptr) {
        *payloadCount = 0;
    }
    return nullptr;
}

extern "C" void nvds_msg2p_release(NvDsMsg2pCtx*, NvDsPayload* payload) {
    if (payload != nullptr) {
        g_free(payload->payload);
        g_free(payload);
    }
}
