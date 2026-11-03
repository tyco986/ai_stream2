#include "routes/types_route.hpp"

#include "constants/generator_pipeline_const.hpp"
#include "constants/types_const.hpp"
#include "response/response.hpp"

void TypesRoute::bind(httplib::Server& server) {
    server.Get(types::PATH, types);
}

void TypesRoute::types(const httplib::Request&, httplib::Response& response) {
    nlohmann::json data = nlohmann::json::object();
    for (const auto& item : generator_pipeline::GENERATOR_PIPELINE_MAP) {
        data[item.first] = item.second;
    }
    Response::ok(response, data);
}
