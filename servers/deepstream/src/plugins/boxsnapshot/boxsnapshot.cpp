#include "plugins/boxsnapshot/boxsnapshot.hpp"

#include "constants/boxsnapshot_const.hpp"

#include <cstring>

extern "C" gpointer boxSnapshotMetaCopy(gpointer data, gpointer userData);
extern "C" void boxSnapshotMetaRelease(gpointer data, gpointer userData);

BoxSnapshotMeta* BoxSnapshot::create(NvDsFrameMeta* frame) {
    guint numBoxes = 0;
    guint maskBytes = 0;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
            auto* object = static_cast<NvDsObjectMeta*>(item->data);
            if (object != nullptr) {
                numBoxes += 1;
                if (object->mask_params.data != nullptr && object->mask_params.size > 0) {
                    maskBytes += object->mask_params.size;
                }
            }
        }
    }
    auto* meta = static_cast<BoxSnapshotMeta*>(g_malloc0(sizeof(BoxSnapshotMeta)));
    meta->num_boxes = numBoxes;
    meta->mask_bytes = maskBytes;
    if (numBoxes > 0) {
        meta->boxes = static_cast<BoxSnapshotBox*>(g_malloc0(static_cast<gsize>(numBoxes) * sizeof(BoxSnapshotBox)));
    }
    if (maskBytes > 0) {
        meta->masks = static_cast<gfloat*>(g_malloc(maskBytes));
    }
    guint index = 0;
    guint offset = 0;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
            auto* object = static_cast<NvDsObjectMeta*>(item->data);
            if (object != nullptr) {
                BoxSnapshotBox* box = &meta->boxes[index];
                box->left = object->rect_params.left;
                box->top = object->rect_params.top;
                box->width = object->rect_params.width;
                box->height = object->rect_params.height;
                box->confidence = object->confidence;
                box->class_id = object->class_id;
                box->object_id = object->object_id;
                g_strlcpy(box->label, object->obj_label, sizeof(box->label));
                if (object->mask_params.data != nullptr && object->mask_params.size > 0) {
                    std::memcpy(reinterpret_cast<unsigned char*>(meta->masks) + offset, object->mask_params.data, object->mask_params.size);
                    box->mask_offset = offset;
                    box->mask_size = object->mask_params.size;
                    box->mask_width = object->mask_params.width;
                    box->mask_height = object->mask_params.height;
                    box->mask_threshold = object->mask_params.threshold;
                    offset += object->mask_params.size;
                }
                index += 1;
            }
        }
    }
    return meta;
}

void BoxSnapshot::attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    BoxSnapshotMeta* snapshot = create(frame);
    NvDsMetaList* item = frame->frame_user_meta_list;
    while (item != nullptr) {
        NvDsMetaList* next = item->next;
        auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
        if (userMeta != nullptr && userMeta->base_meta.meta_type == boxsnapshot::USER_META) {
            nvds_remove_user_meta_from_frame(frame, userMeta);
        }
        item = next;
    }
    NvDsUserMeta* userMeta = nvds_acquire_user_meta_from_pool(batch);
    if (userMeta != nullptr) {
        userMeta->user_meta_data = snapshot;
        userMeta->base_meta.meta_type = boxsnapshot::USER_META;
        userMeta->base_meta.copy_func = boxSnapshotMetaCopy;
        userMeta->base_meta.release_func = boxSnapshotMetaRelease;
        nvds_add_user_meta_to_frame(frame, userMeta);
    } else {
        release(snapshot);
    }
}

BoxSnapshotMeta* BoxSnapshot::copy(const BoxSnapshotMeta* src) {
    BoxSnapshotMeta* dst = nullptr;
    if (src != nullptr) {
        dst = static_cast<BoxSnapshotMeta*>(g_malloc0(sizeof(BoxSnapshotMeta)));
        dst->num_boxes = src->num_boxes;
        dst->mask_bytes = src->mask_bytes;
        if (src->num_boxes > 0 && src->boxes != nullptr) {
            const gsize bytes = static_cast<gsize>(src->num_boxes) * sizeof(BoxSnapshotBox);
            dst->boxes = static_cast<BoxSnapshotBox*>(g_malloc(bytes));
            std::memcpy(dst->boxes, src->boxes, bytes);
        }
        if (src->mask_bytes > 0 && src->masks != nullptr) {
            dst->masks = static_cast<gfloat*>(g_malloc(src->mask_bytes));
            std::memcpy(dst->masks, src->masks, src->mask_bytes);
        }
    }
    return dst;
}

void BoxSnapshot::release(BoxSnapshotMeta* meta) {
    if (meta != nullptr) {
        g_free(meta->boxes);
        g_free(meta->masks);
        g_free(meta);
    }
}

extern "C" gpointer boxSnapshotMetaCopy(gpointer data, gpointer userData) {
    (void)userData;
    const BoxSnapshotMeta* src = nullptr;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        src = static_cast<const BoxSnapshotMeta*>(userMeta->user_meta_data);
    }
    return BoxSnapshot::copy(src);
}

extern "C" void boxSnapshotMetaRelease(gpointer data, gpointer userData) {
    (void)userData;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        BoxSnapshot::release(static_cast<BoxSnapshotMeta*>(userMeta->user_meta_data));
        userMeta->user_meta_data = nullptr;
    }
}
