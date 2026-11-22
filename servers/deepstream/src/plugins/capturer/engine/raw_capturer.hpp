#pragma once

#include "plugins/capturer/engine/capturer.hpp"
#include "plugins/capturer/engine/png_writer.hpp"

class RawCapturer : public Capturer {
public:
    RawCapturer();
    ~RawCapturer() override;

protected:
    const char* dumpKey() const override;
    bool save(const CaptureJob& job) override;

private:
    PngWriter png;
};
