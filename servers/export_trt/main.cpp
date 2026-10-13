#include "constants/service_main.hpp"
#include "routes/exporter.hpp"
#include "routes/health.hpp"
#include "routes/types.hpp"

#include "libs/httplib.h"

int main() {
    httplib::Server server;
    HealthRoute::bind(server);
    TypesRoute::bind(server);
    ExportRoute::bind(server);
    const bool listened = server.listen(service_main::HOST, service_main::PORT);
    return listened ? 0 : 1;
}
