#pragma once

#include "plugins/capturer/engine/capturer.hpp"
#include "plugins/capturer/engine/png_writer.hpp"

class RawCapturer : public Capturer {
public:
    RawCapturer(const char* captureCodes, const char* output);
    ~RawCapturer() override;

protected:
    bool save(const CaptureJob& job) override;

private:
    PngWriter png;
};
