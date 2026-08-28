#pragma once

#include <algorithm>

class RectExpand {
 public:
  RectExpand(int infer_width, int infer_height, float padding)
  {
    this->infer_width = infer_width;
    this->infer_height = infer_height;
    this->padding = padding;
  }

  void expand(
      float left,
      float top,
      float width,
      float height,
      int frame_width,
      int frame_height,
      float *out_left,
      float *out_top,
      float *out_width,
      float *out_height) const
  {
    float aspect = infer_width / static_cast<float>(infer_height);
    float center_x = left + width * 0.5f;
    float center_y = top + height * 0.5f;
    float scale_w = width * padding;
    float scale_h = height * padding;
    if (scale_w > aspect * scale_h) {
      scale_h = scale_w / aspect;
    } else {
      scale_w = scale_h * aspect;
    }
    float expanded_left = std::max(0.0f, center_x - scale_w * 0.5f);
    float expanded_top = std::max(0.0f, center_y - scale_h * 0.5f);
    float expanded_right = std::min(static_cast<float>(frame_width), center_x + scale_w * 0.5f);
    float expanded_bottom = std::min(static_cast<float>(frame_height), center_y + scale_h * 0.5f);
    *out_left = expanded_left;
    *out_top = expanded_top;
    *out_width = std::max(2.0f, expanded_right - expanded_left);
    *out_height = std::max(2.0f, expanded_bottom - expanded_top);
  }

  void even_src(
      float left,
      float top,
      float width,
      float height,
      int *src_left,
      int *src_top,
      int *src_width,
      int *src_height) const
  {
    int aligned_left = round_up_2(static_cast<int>(left));
    int aligned_top = round_up_2(static_cast<int>(top));
    int aligned_width = round_down_2(static_cast<int>(width));
    int aligned_height = round_down_2(static_cast<int>(height));
    if (aligned_width < 2) {
      aligned_width = 2;
    }
    if (aligned_height < 2) {
      aligned_height = 2;
    }
    *src_left = aligned_left;
    *src_top = aligned_top;
    *src_width = aligned_width;
    *src_height = aligned_height;
  }

  void letterbox(
      int src_width,
      int src_height,
      int *dest_width,
      int *dest_height,
      int *offset_left,
      int *offset_top) const
  {
    float fit_height = infer_width * src_height / static_cast<float>(src_width);
    int box_width = infer_width;
    int box_height = static_cast<int>(fit_height);
    if (fit_height > infer_height) {
      box_width = static_cast<int>(infer_height * src_width / static_cast<float>(src_height));
      box_height = infer_height;
    }
    *dest_width = box_width;
    *dest_height = box_height;
    *offset_left = (infer_width - box_width) / 2;
    *offset_top = (infer_height - box_height) / 2;
  }

  int infer_width;
  int infer_height;
  float padding;

 private:
  int round_up_2(int value) const
  {
    return (value + 1) & ~1;
  }

  int round_down_2(int value) const
  {
    return value & ~1;
  }
};
