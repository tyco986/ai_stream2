#include "routes/generate_route.hpp"

#include "constants/generate_const.hpp"
#include "constants/history_const.hpp"
#include "manager/generator_manager.hpp"
#include "response/response.hpp"
#include "services/history_service.hpp"

#include <filesystem>

void GenerateRoute::bind(httplib::Server& server) {
    server.Post(generate::PATH, generate);
}

void GenerateRoute::generate(const httplib::Request& request, httplib::Response& response) {
    const httplib::FormData yamlFile = request.form.get_file(generate::FIELD_YAML);
    const nlohmann::json payload = History::yaml2json(YAML::Load(yamlFile.content));
    const std::string type = payload.at(generate::KEY_GENERATOR).get<std::string>();
    const std::filesystem::path stem = std::filesystem::path(yamlFile.filename).stem();
    const std::string fileName = stem.string() + history::ZIP_SUFFIX;
    const std::filesystem::path configSaveDir = std::filesystem::path(generate::RUNTIME_ROOT) / stem;
    const std::string archive = GeneratorManager::generate(type, configSaveDir, payload);
    History().save(fileName, archive);
    Response::ok(response, payload, "", fileName, archive);
}
