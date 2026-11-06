#pragma once

#include <cstdint>
#include <string>
#include <vector>

class JpegWriter {
public:
    bool write(const std::string& path, const std::vector<std::uint8_t>& rgb, int width, int height);
};
