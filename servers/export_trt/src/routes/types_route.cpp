#include "routes/types_route.hpp"

#include "constants/types_const.hpp"
#include "response/response.hpp"

#include <iterator>
#include <vector>

#include <nlohmann/json.hpp>

void TypesRoute::bind(httplib::Server& server) {
    server.Get(::types::TYPES_PATH, types);
}

void TypesRoute::types(const httplib::Request&, httplib::Response& response) {
    const std::vector<const char*> items(std::begin(::types::AVAILABLE_TYPES), std::end(::types::AVAILABLE_TYPES));
    const nlohmann::json data{{"items", items}};
    Response::ok(response, data);
}
