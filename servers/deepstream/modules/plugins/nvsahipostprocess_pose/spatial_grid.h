#ifndef __SAHI_POSE_SPATIAL_GRID_H__
#define __SAHI_POSE_SPATIAL_GRID_H__

#include <algorithm>
#include <cmath>
#include <vector>
#include <glib.h>

struct SahiPoseGridRect {
  gfloat left, top, right, bottom;
};

class SahiPoseSpatialGrid {
public:
  void build(const std::vector<SahiPoseGridRect> &boxes, guint count,
             gfloat frame_w, gfloat frame_h)
  {
    if (count == 0)
      return;

    gfloat max_dim = 0.0f;
    for (guint i = 0; i < count; i++) {
      gfloat w = boxes[i].right - boxes[i].left;
      gfloat h = boxes[i].bottom - boxes[i].top;
      max_dim = std::max(max_dim, std::max(w, h));
    }

    cell_size_ = std::max(max_dim, 1.0f);
    cols_ = static_cast<guint>(std::ceil(frame_w / cell_size_)) + 1;
    rows_ = static_cast<guint>(std::ceil(frame_h / cell_size_)) + 1;

    guint total_cells = cols_ * rows_;
    cells_.clear();
    cells_.resize(total_cells);

    for (guint i = 0; i < count; i++) {
      guint c0, r0, c1, r1;
      cell_range(boxes[i], c0, r0, c1, r1);
      for (guint r = r0; r <= r1; r++) {
        for (guint c = c0; c <= c1; c++)
          cells_[r * cols_ + c].push_back(i);
      }
    }

    stamp_.assign(count, 0);
    gen_ = 0;
  }

  void query(const SahiPoseGridRect &box, std::vector<guint> &result) const
  {
    result.clear();
    guint c0, r0, c1, r1;
    cell_range(box, c0, r0, c1, r1);
    ++gen_;
    for (guint r = r0; r <= r1; r++) {
      for (guint c = c0; c <= c1; c++) {
        const auto &cell = cells_[r * cols_ + c];
        for (guint idx : cell) {
          if (stamp_[idx] != gen_) {
            stamp_[idx] = gen_;
            result.push_back(idx);
          }
        }
      }
    }
  }

private:
  void cell_range(const SahiPoseGridRect &box,
                  guint &c0, guint &r0, guint &c1, guint &r1) const
  {
    c0 = static_cast<guint>(std::max(0.0f, box.left / cell_size_));
    r0 = static_cast<guint>(std::max(0.0f, box.top / cell_size_));
    c1 = std::min(static_cast<guint>(box.right / cell_size_), cols_ - 1);
    r1 = std::min(static_cast<guint>(box.bottom / cell_size_), rows_ - 1);
  }

  gfloat cell_size_ = 1.0f;
  guint cols_ = 0;
  guint rows_ = 0;
  std::vector<std::vector<guint>> cells_;
  mutable std::vector<guint> stamp_;
  mutable guint gen_ = 0;
};

#endif
