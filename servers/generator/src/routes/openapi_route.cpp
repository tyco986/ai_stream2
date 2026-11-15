#include "routes/openapi_route.hpp"

#include "constants/openapi_const.hpp"

void OpenApiRoute::bind(httplib::Server& server) {
    server.Get(openapi::SPEC_PATH, spec);
    server.Get(openapi::DOCS_PATH, docs);
}

void OpenApiRoute::spec(const httplib::Request&, httplib::Response& response) {
    response.set_content(openapi::SPEC, "application/json");
}

void OpenApiRoute::docs(const httplib::Request&, httplib::Response& response) {
    response.set_content(openapi::DOCS, "text/html");
}
