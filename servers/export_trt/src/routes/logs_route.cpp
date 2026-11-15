#include "routes/logs_route.hpp"

#include "constants/logger_const.hpp"
#include "logger/request_logger.hpp"
#include "response/response.hpp"
#include "services/logs_service.hpp"

void LogsRoute::bind(httplib::Server& server) {
    server.Post(request_log::PATH_ENABLE, enable);
    server.Post(request_log::PATH_DISABLE, disable);
    server.Delete(request_log::PATH, remove);
}

void LogsRoute::enable(const httplib::Request&, httplib::Response& response) {
    RequestLogger::setEnabled(true);
    Response::ok(response);
}

void LogsRoute::disable(const httplib::Request&, httplib::Response& response) {
    RequestLogger::setEnabled(false);
    Response::ok(response);
}

void LogsRoute::remove(const httplib::Request&, httplib::Response& response) {
    Logs().remove();
    Response::ok(response);
}
