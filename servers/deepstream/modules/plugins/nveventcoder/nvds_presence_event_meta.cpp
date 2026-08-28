#include "nvds_presence_event_meta.h"

#include <cstring>

extern "C" gpointer nvds_presence_event_meta_copy(gpointer data, gpointer user_data)
{
  (void)user_data;
  NvDsPresenceEventMeta *dst = nullptr;
  auto *user_meta = static_cast<NvDsUserMeta *>(data);
  NvDsPresenceEventMeta *src = nullptr;
  if (user_meta != nullptr) {
    src = static_cast<NvDsPresenceEventMeta *>(user_meta->user_meta_data);
  }
  if (src != nullptr) {
    dst = static_cast<NvDsPresenceEventMeta *>(g_malloc0(sizeof(NvDsPresenceEventMeta)));
    std::memcpy(dst, src, sizeof(NvDsPresenceEventMeta));
  }
  return dst;
}

extern "C" void nvds_presence_event_meta_release(gpointer data, gpointer user_data)
{
  (void)user_data;
  auto *user_meta = static_cast<NvDsUserMeta *>(data);
  if (user_meta != nullptr) {
    g_free(user_meta->user_meta_data);
    user_meta->user_meta_data = nullptr;
  }
}
