#include "constants/service_main_const.hpp"
#include "logger/request_logger.hpp"
#include "routes/generate_route.hpp"
#include "routes/health_route.hpp"
#include "routes/history_route.hpp"
#include "routes/logs_route.hpp"
#include "routes/types_route.hpp"

#include "libs/httplib.h"

int main() {
    httplib::Server server;
    server.set_pre_routing_handler(RequestLogger::before);
    server.set_exception_handler(RequestLogger::onException);
    server.set_logger(RequestLogger::write);
    HealthRoute::bind(server);
    TypesRoute::bind(server);
    GenerateRoute::bind(server);
    HistoryRoute::bind(server);
    LogsRoute::bind(server);
    const bool listened = server.listen(service_main::HOST, service_main::PORT);
    return listened ? 0 : 1;
}
