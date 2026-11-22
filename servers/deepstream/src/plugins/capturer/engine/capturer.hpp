#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gstnvdsmeta.h"
#include "nvbufsurface.h"

struct CaptureBox {
    float left;
    float top;
    float width;
    float height;
    int classId;
    std::string label;
};

struct CaptureJob {
    std::vector<std::uint8_t> rgb;
    std::vector<CaptureBox> boxes;
    int width;
    int height;
    std::string rawPath;
    std::string visualizationPath;
};

class Capturer {
public:
    Capturer();
    virtual ~Capturer();

    Capturer(const Capturer&) = delete;
    Capturer& operator=(const Capturer&) = delete;

    bool capture(NvBufSurface* surface, NvDsFrameMeta* frame);

protected:
    virtual const char* dumpKey() const = 0;
    virtual void prepare(CaptureJob* job, NvDsFrameMeta* frame);
    virtual bool save(const CaptureJob& job) = 0;
    void stop();

private:
    std::string eventPath(NvDsFrameMeta* frame, const char* key) const;
    bool shouldDump(NvDsFrameMeta* frame) const;
    bool copyRgb(NvBufSurface* surface, unsigned int batchId, std::vector<std::uint8_t>* rgb, int* width, int* height);
    bool copyCuda(NvBufSurfaceParams* params, int channels, bool swapRb, std::vector<std::uint8_t>* rgb);
    bool copyMapped(
        NvBufSurface* surface,
        unsigned int batchId,
        NvBufSurfaceParams* params,
        int channels,
        bool swapRb,
        std::vector<std::uint8_t>* rgb);
    void packRgb(
        const std::uint8_t* src,
        unsigned int pitch,
        int width,
        int height,
        int channels,
        bool swapRb,
        std::vector<std::uint8_t>* rgb);
    void enqueue(CaptureJob job);
    void writerLoop();

    std::atomic<bool> stopWriter;
    std::mutex writerMutex;
    std::condition_variable writerCv;
    std::deque<CaptureJob> writerQueue;
    std::thread writer;
};
