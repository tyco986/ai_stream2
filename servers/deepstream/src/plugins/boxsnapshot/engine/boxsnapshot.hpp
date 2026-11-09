#pragma once

#include <glib.h>

#include "gstnvdsmeta.h"

struct BoxSnapshotBox {
    gfloat left;
    gfloat top;
    gfloat width;
    gfloat height;
    gfloat confidence;
    gint class_id;
    guint64 object_id;
    gint64 snapshot_key;
    gchar label[64];
    guint mask_offset;
    guint mask_size;
    guint mask_width;
    guint mask_height;
    gfloat mask_threshold;
};

struct BoxSnapshotMeta {
    guint num_boxes;
    BoxSnapshotBox* boxes;
    guint mask_bytes;
    gfloat* masks;
};

class BoxSnapshot {
public:
    static BoxSnapshotMeta* create(NvDsFrameMeta* frame);
    static void attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame);
    static BoxSnapshotMeta* copy(const BoxSnapshotMeta* src);
    static void release(BoxSnapshotMeta* meta);
};
