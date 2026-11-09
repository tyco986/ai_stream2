#include "plugins/capturer/engine/raw_capturer.hpp"

#include <format>

#include <glib.h>

RawCapturer::RawCapturer(const char* captureCodes) : Capturer(captureCodes) {}

RawCapturer::~RawCapturer() {
    stop();
}

bool RawCapturer::save(const CaptureJob& job) {
    const std::string path = std::format("{}/images/raw_{:03d}_{:08d}.png", outputRoot(), job.padIndex, job.captureId);
    const bool ok = png.write(path, job.rgb, job.width, job.height);
    if (!ok) {
        g_warning("rawcapturer png failed path=%s", path.c_str());
    }
    return ok;
}
