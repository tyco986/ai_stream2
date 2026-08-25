#include "gstnvbboxsnapshot.h"

#include <cstring>

#include <gst/video/video.h>

#include "gstnvdsmeta.h"
#include "nvds_bbox_snapshot_meta.h"

GST_DEBUG_CATEGORY_STATIC(gst_nvbboxsnapshot_debug);
#define GST_CAT_DEFAULT gst_nvbboxsnapshot_debug

#define NVBBOXSNAPSHOT_NVMM_CAPS \
  GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVBBOXSNAPSHOT_NVMM_CAPS));
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(NVBBOXSNAPSHOT_NVMM_CAPS));

#define gst_nvbboxsnapshot_parent_class parent_class
G_DEFINE_TYPE(GstNvBboxSnapshot, gst_nvbboxsnapshot, GST_TYPE_BASE_TRANSFORM);

static GstFlowReturn gst_nvbboxsnapshot_transform_ip(GstBaseTransform *, GstBuffer *);

extern "C" gpointer nvds_bbox_snapshot_meta_copy(gpointer data, gpointer user_data)
{
  (void)user_data;
  NvDsBboxSnapshotMeta *src = static_cast<NvDsBboxSnapshotMeta *>(data);
  NvDsBboxSnapshotMeta *dst = nullptr;
  if (src != nullptr) {
    dst = static_cast<NvDsBboxSnapshotMeta *>(g_malloc0(sizeof(NvDsBboxSnapshotMeta)));
    dst->num_boxes = src->num_boxes;
    if (src->num_boxes > 0 && src->boxes != nullptr) {
      gsize bytes = static_cast<gsize>(src->num_boxes) * sizeof(NvDsBboxSnapshotBox);
      dst->boxes = static_cast<NvDsBboxSnapshotBox *>(g_malloc(bytes));
      std::memcpy(dst->boxes, src->boxes, bytes);
    }
  }
  return dst;
}

extern "C" void nvds_bbox_snapshot_meta_release(gpointer data, gpointer user_data)
{
  (void)user_data;
  NvDsBboxSnapshotMeta *meta = static_cast<NvDsBboxSnapshotMeta *>(data);
  if (meta != nullptr) {
    g_free(meta->boxes);
    g_free(meta);
  }
}

static guint count_objects(NvDsFrameMeta *frame_meta)
{
  guint count = 0;
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    if (item->data != nullptr) {
      count += 1;
    }
  }
  return count;
}

static void fill_boxes(NvDsFrameMeta *frame_meta, NvDsBboxSnapshotBox *boxes)
{
  guint index = 0;
  for (NvDsMetaList *item = frame_meta->obj_meta_list; item != nullptr; item = item->next) {
    auto *object_meta = static_cast<NvDsObjectMeta *>(item->data);
    if (object_meta != nullptr) {
      NvDsBboxSnapshotBox *box = &boxes[index];
      box->left = object_meta->rect_params.left;
      box->top = object_meta->rect_params.top;
      box->width = object_meta->rect_params.width;
      box->height = object_meta->rect_params.height;
      box->confidence = object_meta->confidence;
      box->class_id = object_meta->class_id;
      box->object_id = object_meta->object_id;
      index += 1;
    }
  }
}

static void strip_snapshot_meta(NvDsFrameMeta *frame_meta)
{
  NvDsMetaList *item = frame_meta->frame_user_meta_list;
  while (item != nullptr) {
    NvDsMetaList *next = item->next;
    auto *user_meta = static_cast<NvDsUserMeta *>(item->data);
    if (user_meta != nullptr &&
        user_meta->base_meta.meta_type == NVDS_BBOX_SNAPSHOT_USER_META) {
      nvds_remove_user_meta_from_frame(frame_meta, user_meta);
    }
    item = next;
  }
}

static void attach_snapshot(NvDsBatchMeta *batch_meta, NvDsFrameMeta *frame_meta)
{
  guint num_boxes = count_objects(frame_meta);
  auto *snapshot = static_cast<NvDsBboxSnapshotMeta *>(g_malloc0(sizeof(NvDsBboxSnapshotMeta)));
  snapshot->num_boxes = num_boxes;
  if (num_boxes > 0) {
    snapshot->boxes = static_cast<NvDsBboxSnapshotBox *>(
        g_malloc(static_cast<gsize>(num_boxes) * sizeof(NvDsBboxSnapshotBox)));
    fill_boxes(frame_meta, snapshot->boxes);
  }
  strip_snapshot_meta(frame_meta);
  NvDsUserMeta *user_meta = nvds_acquire_user_meta_from_pool(batch_meta);
  if (user_meta != nullptr) {
    user_meta->user_meta_data = snapshot;
    user_meta->base_meta.meta_type = NVDS_BBOX_SNAPSHOT_USER_META;
    user_meta->base_meta.copy_func = nvds_bbox_snapshot_meta_copy;
    user_meta->base_meta.release_func = nvds_bbox_snapshot_meta_release;
    nvds_add_user_meta_to_frame(frame_meta, user_meta);
  } else {
    nvds_bbox_snapshot_meta_release(snapshot, nullptr);
  }
}

static void
gst_nvbboxsnapshot_class_init(GstNvBboxSnapshotClass *klass)
{
  GstElementClass *ge = (GstElementClass *)klass;
  GstBaseTransformClass *bt = (GstBaseTransformClass *)klass;

  bt->transform_ip = GST_DEBUG_FUNCPTR(gst_nvbboxsnapshot_transform_ip);

  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&src_tmpl));
  gst_element_class_add_pad_template(ge, gst_static_pad_template_get(&sink_tmpl));
  gst_element_class_set_details_simple(
      ge, "NvBboxSnapshot", "Filter/Metadata",
      "Copy object boxes into frame user meta", "ai_stream2");
  GST_DEBUG_CATEGORY_INIT(gst_nvbboxsnapshot_debug, "nvbboxsnapshot", 0, "nvbboxsnapshot");
}

static void
gst_nvbboxsnapshot_init(GstNvBboxSnapshot *self)
{
  GstBaseTransform *bt = GST_BASE_TRANSFORM(self);
  gst_base_transform_set_in_place(bt, TRUE);
  gst_base_transform_set_passthrough(bt, FALSE);
}

static GstFlowReturn
gst_nvbboxsnapshot_transform_ip(GstBaseTransform *btrans, GstBuffer *inbuf)
{
  (void)btrans;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(inbuf);
  if (batch_meta != nullptr) {
    nvds_acquire_meta_lock(batch_meta);
    for (NvDsMetaList *item = batch_meta->frame_meta_list; item != nullptr; item = item->next) {
      auto *frame_meta = static_cast<NvDsFrameMeta *>(item->data);
      if (frame_meta != nullptr) {
        attach_snapshot(batch_meta, frame_meta);
      }
    }
    nvds_release_meta_lock(batch_meta);
  }
  return GST_FLOW_OK;
}
