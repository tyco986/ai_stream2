#include "routes/types.hpp"

#include "constants/types.hpp"
#include "response/response.hpp"

#include <iterator>

#include <nlohmann/json.hpp>

void TypesRoute::bind(httplib::Server& server) {
    server.Get(::types::TYPES_PATH, types);
}

void TypesRoute::types(const httplib::Request&, httplib::Response& response) {
    const nlohmann::json data{
        {"items", nlohmann::json(std::begin(::types::AVAILABLE_TYPES), std::end(::types::AVAILABLE_TYPES))},
    };
    http::Response::ok(response, data);
}
