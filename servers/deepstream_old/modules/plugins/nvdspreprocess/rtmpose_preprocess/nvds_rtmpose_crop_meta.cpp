#include "nvds_rtmpose_crop_meta.h"

#include <cstring>

extern "C" gpointer nvds_rtmpose_crop_meta_copy(gpointer data, gpointer user_data)
{
  (void)user_data;
  NvDsRtmposeCropMeta *dst = nullptr;
  auto *user_meta = static_cast<NvDsUserMeta *>(data);
  NvDsRtmposeCropMeta *src = nullptr;
  if (user_meta != nullptr) {
    src = static_cast<NvDsRtmposeCropMeta *>(user_meta->user_meta_data);
  }
  if (src != nullptr) {
    dst = static_cast<NvDsRtmposeCropMeta *>(g_malloc0(sizeof(NvDsRtmposeCropMeta)));
    std::memcpy(dst, src, sizeof(NvDsRtmposeCropMeta));
  }
  return dst;
}

extern "C" void nvds_rtmpose_crop_meta_release(gpointer data, gpointer user_data)
{
  (void)user_data;
  auto *user_meta = static_cast<NvDsUserMeta *>(data);
  if (user_meta != nullptr) {
    g_free(user_meta->user_meta_data);
    user_meta->user_meta_data = nullptr;
  }
}
