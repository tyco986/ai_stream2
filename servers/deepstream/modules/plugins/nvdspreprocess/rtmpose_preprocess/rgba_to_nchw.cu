#include "rgba_to_nchw.h"

#include <cuda_runtime.h>

__global__ void rgba_to_nchw_kernel(
    const unsigned char *src,
    int src_pitch,
    int width,
    int height,
    float *dst,
    float scale,
    float offset_r,
    float offset_g,
    float offset_b)
{
  int x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
  int y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
  if (x >= width || y >= height) {
    return;
  }
  const unsigned char *pixel = src + y * src_pitch + x * 4;
  int hw = width * height;
  int index = y * width + x;
  dst[0 * hw + index] = (static_cast<float>(pixel[0]) - offset_r) * scale;
  dst[1 * hw + index] = (static_cast<float>(pixel[1]) - offset_g) * scale;
  dst[2 * hw + index] = (static_cast<float>(pixel[2]) - offset_b) * scale;
}

void rgba_to_nchw(
    const unsigned char *src,
    int src_pitch,
    int width,
    int height,
    float *dst,
    float scale,
    float offset_r,
    float offset_g,
    float offset_b)
{
  dim3 block(16, 16);
  dim3 grid((width + 15) / 16, (height + 15) / 16);
  rgba_to_nchw_kernel<<<grid, block>>>(
      src,
      src_pitch,
      width,
      height,
      dst,
      scale,
      offset_r,
      offset_g,
      offset_b);
  cudaDeviceSynchronize();
}
