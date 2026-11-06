#pragma once

#include "plugins/capturer/engine/capturer.hpp"
#include "plugins/capturer/engine/png_writer.hpp"

class RawCapturer : public Capturer {
public:
    explicit RawCapturer(const char* captureCodes);
    ~RawCapturer() override;

protected:
    bool save(const CaptureJob& job) override;

private:
    PngWriter png;
};
