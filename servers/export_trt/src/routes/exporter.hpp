#pragma once

#include "libs/httplib.h"

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class ExportRoute {
public:
    static void bind(httplib::Server& server);

private:
    static void exportModel(const httplib::Request& request, httplib::Response& response);
    static nlohmann::json yaml2json(const YAML::Node& node);
};
