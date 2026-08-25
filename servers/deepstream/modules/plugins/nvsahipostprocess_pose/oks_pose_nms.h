#ifndef __SAHI_POSE_OKS_NMS_H__
#define __SAHI_POSE_OKS_NMS_H__

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <vector>
#include "gstnvsahipostprocess_pose.h"
#include "spatial_grid.h"

static const gfloat SAHI_COCO17_SIGMAS[17] = {
    0.026f, 0.025f, 0.025f, 0.035f, 0.035f,
    0.079f, 0.079f, 0.072f, 0.072f, 0.062f,
    0.062f, 0.107f, 0.107f, 0.087f, 0.087f,
    0.089f, 0.089f};

static inline bool
pose_det_compare(const std::vector<SahiPoseDetection> &dets, guint a, guint b)
{
  bool better = dets[a].score > dets[b].score;
  if (dets[a].score == dets[b].score) {
    auto ca = std::tie(dets[a].left, dets[a].top, dets[a].right, dets[a].bottom);
    auto cb = std::tie(dets[b].left, dets[b].top, dets[b].right, dets[b].bottom);
    better = ca < cb;
  }
  return better;
}

struct PoseScoreOrder {
  const std::vector<SahiPoseDetection> *dets;
  bool operator()(guint a, guint b) const
  {
    return pose_det_compare(*dets, a, b);
  }
};

static inline gfloat
pose_keypoint_var(guint k, guint num_keypoints)
{
  gfloat sigma = 0.05f;
  if (num_keypoints == 17 && k < 17)
    sigma = SAHI_COCO17_SIGMAS[k];
  return (sigma * 2.0f) * (sigma * 2.0f);
}

static inline gfloat
compute_pose_oks(const SahiPoseDetection &a, const SahiPoseDetection &b,
                 gfloat vis_thr)
{
  gfloat oks = 0.0f;
  if (a.kpts && b.kpts && a.num_keypoints == b.num_keypoints &&
      a.num_keypoints > 0) {
    const guint kcount = a.num_keypoints;
    const gfloat scale = (a.area + b.area) * 0.5f + 1e-9f;
    gfloat sum = 0.0f;
    guint vis = 0;
    for (guint k = 0; k < kcount; k++) {
      const gfloat sa = a.kpts[k * 3 + 2];
      const gfloat sb = b.kpts[k * 3 + 2];
      if (sa > vis_thr && sb > vis_thr) {
        const gfloat dx = a.kpts[k * 3 + 0] - b.kpts[k * 3 + 0];
        const gfloat dy = a.kpts[k * 3 + 1] - b.kpts[k * 3 + 1];
        const gfloat var = pose_keypoint_var(k, kcount);
        const gfloat energy = (dx * dx + dy * dy) / var / scale / 2.0f;
        sum += std::exp(-energy);
        vis++;
      }
    }
    oks = sum / (gfloat)std::max(vis, (guint)1);
  }
  return oks;
}

static inline void
run_oks_nms_on_group(
    std::vector<SahiPoseDetection> &dets,
    std::vector<uint8_t> &suppressed,
    const SahiPoseSpatialGrid &grid,
    const std::vector<guint> &idx_set,
    gfloat oks_thr, gfloat vis_thr, gboolean agnostic)
{
  std::vector<guint> candidates;

  for (guint ii = 0; ii < idx_set.size(); ii++) {
    guint i = idx_set[ii];
    if (suppressed[i])
      continue;

    grid.query({dets[i].left, dets[i].top, dets[i].right, dets[i].bottom},
               candidates);

    for (guint c : candidates) {
      if (c == i || suppressed[c])
        continue;
      if (!agnostic && dets[i].class_id != dets[c].class_id)
        continue;
      if (pose_det_compare(dets, c, i))
        continue;
      gfloat oks = compute_pose_oks(dets[i], dets[c], vis_thr);
      if (oks > oks_thr)
        suppressed[c] = 1;
    }
  }
}

#endif
