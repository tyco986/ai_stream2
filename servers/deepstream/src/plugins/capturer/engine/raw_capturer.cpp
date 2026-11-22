#include "plugins/capturer/engine/raw_capturer.hpp"

#include <glib.h>

#include "constants/collector_const.hpp"

RawCapturer::RawCapturer() : Capturer() {}

RawCapturer::~RawCapturer() {
    stop();
}

const char* RawCapturer::dumpKey() const {
    return collector::KEY_RAW_PATH;
}

bool RawCapturer::save(const CaptureJob& job) {
    const std::string path = job.rawPath;
    const bool ok = png.write(path, job.rgb, job.width, job.height);
    if (!ok) {
        g_warning("rawcapturer png failed path=%s", path.c_str());
    }
    return ok;
}
