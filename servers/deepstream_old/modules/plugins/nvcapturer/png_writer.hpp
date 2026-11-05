#pragma once

#include <cstdint>
#include <string>
#include <vector>

class PngWriter {
 public:
  PngWriter();

  bool write_rgb(
      const std::string &path,
      const std::vector<uint8_t> &rgb,
      int width,
      int height);
};
