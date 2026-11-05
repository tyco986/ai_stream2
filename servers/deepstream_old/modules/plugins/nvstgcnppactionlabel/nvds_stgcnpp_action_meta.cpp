#include "nvds_stgcnpp_action_meta.h"

#include <cstring>

extern "C" gpointer nvds_stgcnpp_action_meta_copy(gpointer data, gpointer user_data)
{
  (void)user_data;
  NvDsStgcnppActionMeta *dst = nullptr;
  auto *user_meta = static_cast<NvDsUserMeta *>(data);
  NvDsStgcnppActionMeta *src = nullptr;
  if (user_meta != nullptr) {
    src = static_cast<NvDsStgcnppActionMeta *>(user_meta->user_meta_data);
  }
  if (src != nullptr) {
    dst = static_cast<NvDsStgcnppActionMeta *>(g_malloc0(sizeof(NvDsStgcnppActionMeta)));
    std::memcpy(dst, src, sizeof(NvDsStgcnppActionMeta));
  }
  return dst;
}

extern "C" void nvds_stgcnpp_action_meta_release(gpointer data, gpointer user_data)
{
  (void)user_data;
  auto *user_meta = static_cast<NvDsUserMeta *>(data);
  if (user_meta != nullptr) {
    g_free(user_meta->user_meta_data);
    user_meta->user_meta_data = nullptr;
  }
}
