#pragma once

#include <NvInfer.h>

#include <string>

class TrtLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* message) noexcept override;
    const std::string& errors() const;

private:
    std::string lines;
};
