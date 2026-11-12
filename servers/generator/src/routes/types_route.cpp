#include "routes/types_route.hpp"

#include "constants/generator_pipeline_const.hpp"
#include "constants/types_const.hpp"
#include "response/response.hpp"

void TypesRoute::bind(httplib::Server& server) {
    server.Get(types::PATH, types);
}

void TypesRoute::types(const httplib::Request&, httplib::Response& response) {
    Response::ok(response, generator_pipeline::PIPELINE_TYPES);
}
