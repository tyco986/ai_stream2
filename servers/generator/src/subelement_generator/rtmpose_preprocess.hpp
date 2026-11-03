#pragma once

#include <filesystem>
#include <optional>
#include <string>

class NvdspreprocessRtmposeGenerator {
public:
    NvdspreprocessRtmposeGenerator(
        int batchSize,
        int inferWidth,
        int inferHeight,
        int channels,
        int streamCount,
        std::optional<std::string> tensorName = std::nullopt);

    std::string render() const;
    void write(const std::filesystem::path& savePath);

private:
    int batchSize;
    int inferWidth;
    int inferHeight;
    int channels;
    int streamCount;
    std::string tensorName;
};
