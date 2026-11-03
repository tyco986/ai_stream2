#pragma once

#include <set>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

class NvtrackerParser {
public:
    explicit NvtrackerParser(int interval);

    int alignTrackerDimension(int value) const;
    bool validate(const YAML::Node& tracker, const std::set<int>& pgieClassIds) const;
    std::string formatOperateOnClassIds(const YAML::Node& tracker) const;

    int interval;
    int maxShadowTrackingAge;
    int earlyTerminationAge;
    int probationAge;

private:
    std::vector<int> classIds(const YAML::Node& classId) const;
};
