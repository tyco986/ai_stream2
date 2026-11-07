#include "plugins/nvmsgconv/msgconv_payload.hpp"

#include <cstring>
#include <memory>

#include "constants/dslogger_const.hpp"
#include "nvmsgconv.h"

#if defined(MSGCONV_DET)
#include "plugins/dslogger/engine/det_log_engine.hpp"
#elif defined(MSGCONV_PRESENCE)
#include "plugins/dslogger/engine/presence_log_engine.hpp"
#elif defined(MSGCONV_STGCNPP)
#include "plugins/dslogger/engine/stgcnpp_log_engine.hpp"
#else
#error MSGCONV_DET, MSGCONV_PRESENCE, or MSGCONV_STGCNPP is required
#endif

namespace {

std::unique_ptr<LogEngine> makeEngine() {
#if defined(MSGCONV_DET)
    return std::make_unique<DetLogEngine>(0);
#elif defined(MSGCONV_PRESENCE)
    return std::make_unique<PresenceLogEngine>(0);
#else
    return std::make_unique<StgcnppLogEngine>(0);
#endif
}

}

MsgconvPayload::MsgconvPayload()
    : engine(makeEngine()) {
}

bool MsgconvPayload::publishable(NvDsFrameMeta* frame) const {
#if defined(MSGCONV_PRESENCE)
    return static_cast<PresenceLogEngine*>(engine.get())->active(frame);
#else
    return frame != nullptr;
#endif
}

std::string MsgconvPayload::line(NvDsFrameMeta* frame) const {
    return engine->buildLine(frame, dslogger::LATENCY_NONE);
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
    if (state != nullptr && frame != nullptr && state->publishable(frame)) {
        const std::string line = state->line(frame);
        payload = static_cast<NvDsPayload*>(g_malloc0(sizeof(NvDsPayload)));
        payload->payload = g_malloc(line.size());
        memcpy(payload->payload, line.data(), line.size());
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
