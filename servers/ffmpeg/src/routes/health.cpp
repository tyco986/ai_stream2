#include "routes/health.hpp"

#include "constants/health.hpp"
#include "response/response.hpp"

void HealthRoute::bind(httplib::Server& server) {
    server.Get(::health::HEALTH_PATH, health);
}

void HealthRoute::health(const httplib::Request&, httplib::Response& response) {
    http::Response::ok(response);
}
