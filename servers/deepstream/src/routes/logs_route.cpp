#include "routes/logs_route.hpp"

#include "constants/logger_const.hpp"
#include "constants/pipeline_const.hpp"
#include "response/response.hpp"
#include "services/logs_service.hpp"
#include "services/pipeline_service.hpp"

void LogsRoute::bind(httplib::Server& server) {
    server.Delete(request_log::PATH, remove);
}

void LogsRoute::remove(const httplib::Request&, httplib::Response& response) {
    const bool busy = PipelineService::alive();
    if (busy) {
        Response::error(response, pipeline::STATUS_CONFLICT, pipeline::ERR_LOGS_BUSY);
    }
    if (!busy) {
        Logs().remove();
        Response::ok(response);
    }
}
