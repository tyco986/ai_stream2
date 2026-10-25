#include "constants/service_main_const.hpp"
#include "routes/health_route.hpp"
#include "routes/rtsp_route.hpp"
#include "routes/video_route.hpp"

#include "libs/httplib.h"

int main() {
    httplib::Server server;
    HealthRoute::bind(server);
    RtspRoute::bind(server);
    VideoRoute::bind(server);
    const bool listened = server.listen(service_main::HOST, service_main::PORT);
    return listened ? 0 : 1;
}
