#include "constants/service_main.hpp"
#include "routes/health.hpp"
#include "routes/rtsp.hpp"

#include "libs/httplib.h"

int main() {
    httplib::Server server;
    HealthRoute::bind(server);
    RtspRoute::bind(server);
    const bool listened = server.listen(service_main::HOST, service_main::PORT);
    return listened ? 0 : 1;
}
