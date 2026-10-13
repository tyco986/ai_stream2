#include "routes/exporter.hpp"

#include "constants/exporter.hpp"
#include "response/response.hpp"
#include "services/exporter.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

void ExportRoute::bind(httplib::Server& server) {
    server.Post(::exporter::EXPORT_PATH, exportModel);
}

void ExportRoute::exportModel(const httplib::Request& request, httplib::Response& response) {
    const httplib::FormData upload = request.form.get_file(::exporter::FIELD_INPUT);
    const httplib::FormData config_file = request.form.get_file(::exporter::FIELD_CONFIG);
    const nlohmann::json config = yaml2json(YAML::Load(config_file.content));
    const ExportResult result = Exporter().exportModel(upload.filename, upload.content, config);
    std::ifstream stream(result.archive, std::ios::binary);
    const bool opened = stream.is_open();
    const std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (!opened) {
        throw std::runtime_error("engine zip missing");
    }
    http::Response::ok(response, result.data, "", result.archive.filename().string(), bytes);
}

nlohmann::json ExportRoute::yaml2json(const YAML::Node& node) {
    nlohmann::json value = nullptr;
    if (node.IsMap()) {
        value = nlohmann::json::object();
        for (YAML::const_iterator item = node.begin(); item != node.end(); ++item) {
            value[item->first.as<std::string>()] = yaml2json(item->second);
        }
    }
    if (node.IsSequence()) {
        value = nlohmann::json::array();
        for (const YAML::Node& item : node) {
            value.push_back(yaml2json(item));
        }
    }
    if (node.IsScalar()) {
        bool boolean = false;
        std::int64_t integer = 0;
        double number = 0;
        const bool is_bool = YAML::convert<bool>::decode(node, boolean);
        const bool is_int = YAML::convert<std::int64_t>::decode(node, integer);
        const bool is_double = YAML::convert<double>::decode(node, number);
        value = node.Scalar();
        value = is_bool ? nlohmann::json(boolean) : value;
        value = !is_bool && is_int ? nlohmann::json(integer) : value;
        value = !is_bool && !is_int && is_double ? nlohmann::json(number) : value;
    }
    return value;
}
