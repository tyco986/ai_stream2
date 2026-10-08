#include "constants/main.hpp"
#include "routes/rtsp.hpp"

#include "libs/httplib.h"

int main() {
    httplib::Server server;
    RtspRoute::bind(server);
    const bool listened = server.listen(HOST, PORT);
    return listened ? 0 : 1;
}
