#pragma once

#include <filesystem>
#include <string>

class StgcnppPreprocessGenerator {
public:
    StgcnppPreprocessGenerator(
        int batchSize,
        int clipLen,
        int numJoints,
        int numPerson,
        std::string tensorName,
        int streamCount,
        int patience);

    std::string render() const;
    void write(const std::filesystem::path& savePath);

private:
    int batchSize;
    int clipLen;
    int numJoints;
    int numPerson;
    std::string tensorName;
    int streamCount;
    int patience;
};
