#pragma once

#include <string>

#include "plugins/capturer/engine/capturer.hpp"
#include "plugins/capturer/engine/jpeg_writer.hpp"

class VisCapturer : public Capturer {
public:
    explicit VisCapturer(const char* labelTask);
    ~VisCapturer() override;

protected:
    const char* dumpKey() const override;
    void prepare(CaptureJob* job, NvDsFrameMeta* frame) override;
    bool save(const CaptureJob& job) override;

private:
    bool writeYolo(const std::string& path, const std::vector<CaptureBox>& boxes, int imageWidth, int imageHeight);
    bool writeLabelme(
        const std::string& path,
        const std::vector<CaptureBox>& boxes,
        int imageWidth,
        int imageHeight,
        const std::string& imagePath);
    bool writeLabels(const CaptureJob& job);
    std::string jsonEscape(const std::string& text) const;

    std::string labelTask;
    JpegWriter jpeg;
};
