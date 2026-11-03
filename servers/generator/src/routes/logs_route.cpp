#include "routes/logs_route.hpp"

#include "constants/logger_const.hpp"
#include "response/response.hpp"
#include "services/logs_service.hpp"

void LogsRoute::bind(httplib::Server& server) {
    server.Delete(request_log::PATH, remove);
}

void LogsRoute::remove(const httplib::Request&, httplib::Response& response) {
    Logs().remove();
    Response::ok(response);
}
