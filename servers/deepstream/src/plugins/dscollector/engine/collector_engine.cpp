#include "plugins/dscollector/engine/collector_engine.hpp"

#include <glib.h>

#include <cmath>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>

#include "constants/collector_const.hpp"

extern "C" gpointer collectorMetaCopy(gpointer data, gpointer userData);
extern "C" void collectorMetaRelease(gpointer data, gpointer userData);

CollectorEngine::CollectorEngine(int interval)
    : sampleInterval(interval),
      batchCounter(0) {
    sampleInterval = sampleInterval < 1 ? 1 : sampleInterval;
}

void CollectorEngine::setInterval(int interval) {
    sampleInterval = interval < 1 ? 1 : interval;
    batchCounter = 0;
}

int CollectorEngine::getInterval() const {
    return sampleInterval;
}

double CollectorEngine::latencyFor(NvDsFrameLatencyInfo* info, guint count, NvDsFrameMeta* frame) const {
    double latencyMs = collector::LATENCY_NONE;
    if (info != nullptr && frame != nullptr) {
        for (guint index = 0; index < count; index++) {
            if (info[index].source_id == frame->source_id && static_cast<gint>(info[index].frame_num) == frame->frame_num) {
                latencyMs = info[index].latency;
            }
        }
    }
    return latencyMs;
}

void CollectorEngine::collect(NvDsBatchMeta* batch, NvDsFrameLatencyInfo* info, guint count) {
    const bool sample = batchCounter % sampleInterval == 0;
    batchCounter = sample ? 1 : batchCounter + 1;
    for (NvDsMetaList* item = batch->frame_meta_list; item != nullptr; item = item->next) {
        auto* frame = static_cast<NvDsFrameMeta*>(item->data);
        if (frame != nullptr) {
            NvDsMetaList* meta = frame->frame_user_meta_list;
            while (meta != nullptr) {
                NvDsMetaList* next = meta->next;
                auto* userMeta = static_cast<NvDsUserMeta*>(meta->data);
                if (userMeta != nullptr && userMeta->base_meta.meta_type == collector::USER_META) {
                    nvds_remove_user_meta_from_frame(frame, userMeta);
                }
                meta = next;
            }
            NvDsUserMeta* userMeta = sample ? nvds_acquire_user_meta_from_pool(batch) : nullptr;
            if (userMeta != nullptr) {
                userMeta->user_meta_data = g_strdup(buildLine(frame, latencyFor(info, count, frame)).c_str());
                userMeta->base_meta.meta_type = collector::USER_META;
                userMeta->base_meta.copy_func = collectorMetaCopy;
                userMeta->base_meta.release_func = collectorMetaRelease;
                nvds_add_user_meta_to_frame(frame, userMeta);
            }
        }
    }
}

void CollectorEngine::appendExtra(std::ostringstream&, NvDsFrameMeta*) const {
}

std::string CollectorEngine::escapeLabel(const char* label) const {
    std::string out;
    const char* src = label != nullptr ? label : "";
    for (const char* cursor = src; *cursor != '\0'; ++cursor) {
        const char ch = *cursor;
        if (ch == '\\' || ch == '"') {
            out.push_back('\\');
        }
        if (ch != '\n' && ch != '\r') {
            out.push_back(ch);
        }
    }
    return out;
}

void CollectorEngine::appendDetFields(std::ostringstream& json, NvDsObjectMeta* object) const {
    const float left = object->rect_params.left;
    const float top = object->rect_params.top;
    const double logged = std::round(static_cast<double>(object->confidence) * collector::CONF_SCALE) / collector::CONF_SCALE;
    const int objectId = object->object_id == UNTRACKED_OBJECT_ID ? collector::MISSING_OBJECT_ID : static_cast<int>(object->object_id);
    json << static_cast<int>(std::lround(left)) << ","
         << static_cast<int>(std::lround(top)) << ","
         << static_cast<int>(std::lround(left + object->rect_params.width)) << ","
         << static_cast<int>(std::lround(top + object->rect_params.height)) << ","
         << std::fixed << std::setprecision(collector::CONF_DIGITS) << logged << ","
         << object->class_id << ",\"" << escapeLabel(object->obj_label) << "\","
         << objectId;
}

void CollectorEngine::appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const {
    json << "[";
    appendDetFields(json, object);
    json << "]";
}

std::string CollectorEngine::buildLine(NvDsFrameMeta* frame, double latencyMs) const {
    std::ostringstream json;
    const std::time_t now = std::time(nullptr);
    std::tm local {};
    localtime_r(&now, &local);
    char timestamp[collector::TS_SIZE];
    std::strftime(timestamp, sizeof(timestamp), collector::TS_FORMAT, &local);
    json << "{\"ts\":\"" << timestamp << "\""
         << ",\"pad\":" << static_cast<int>(frame->pad_index)
         << ",\"source\":" << static_cast<int>(frame->source_id)
         << ",\"frame\":" << static_cast<int>(frame->frame_num)
         << ",\"latency\":";
    if (latencyMs == collector::LATENCY_NONE) {
        json << collector::JSON_NULL;
    } else {
        json << std::fixed << std::setprecision(collector::CONF_DIGITS) << latencyMs;
    }
    std::map<int, int> classCounts;
    int all = 0;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            classCounts[object->class_id] += 1;
            all += 1;
        }
    }
    json << ",\"num\":{\"all\":" << all;
    for (const auto& entry : classCounts) {
        json << ",\"" << entry.first << "\":" << entry.second;
    }
    json << "}";
    appendExtra(json, frame);
    json << ",\"object\":[";
    bool first = true;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            if (!first) {
                json << ",";
            }
            first = false;
            appendObjectItem(json, object);
        }
    }
    json << "]}";
    return json.str();
}

extern "C" gpointer collectorMetaCopy(gpointer data, gpointer userData) {
    (void)userData;
    gchar* line = nullptr;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr && userMeta->user_meta_data != nullptr) {
        line = g_strdup(static_cast<const char*>(userMeta->user_meta_data));
    }
    return line;
}

extern "C" void collectorMetaRelease(gpointer data, gpointer userData) {
    (void)userData;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        g_free(userMeta->user_meta_data);
        userMeta->user_meta_data = nullptr;
    }
}
