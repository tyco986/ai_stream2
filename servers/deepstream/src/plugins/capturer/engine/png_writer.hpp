#pragma once

#include <cstdint>
#include <string>
#include <vector>

class PngWriter {
public:
    bool write(const std::string& path, const std::vector<std::uint8_t>& rgb, int width, int height);
};
