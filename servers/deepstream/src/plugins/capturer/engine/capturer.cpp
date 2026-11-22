#include "plugins/capturer/engine/capturer.hpp"

#include <string>
#include <utility>

#include <cuda_runtime.h>
#include <glib.h>

#include "constants/collector_const.hpp"
#include "plugins/dscollector/engine/collector_line.hpp"

namespace {

int channelCount(NvBufSurfaceColorFormat format) {
    int count = 3;
    if (format == NVBUF_COLOR_FORMAT_RGBA || format == NVBUF_COLOR_FORMAT_BGRA || format == NVBUF_COLOR_FORMAT_RGBx
        || format == NVBUF_COLOR_FORMAT_BGRx) {
        count = 4;
    }
    return count;
}

bool rgbFormat(NvBufSurfaceColorFormat format) {
    const bool rgb = format == NVBUF_COLOR_FORMAT_RGB || format == NVBUF_COLOR_FORMAT_RGBA || format == NVBUF_COLOR_FORMAT_RGBx;
    return rgb;
}

bool bgrFormat(NvBufSurfaceColorFormat format) {
    const bool bgr = format == NVBUF_COLOR_FORMAT_BGR || format == NVBUF_COLOR_FORMAT_BGRA || format == NVBUF_COLOR_FORMAT_BGRx;
    return bgr;
}

bool cudaMemory(NvBufSurfaceMemType memType) {
    const bool cuda = memType == NVBUF_MEM_CUDA_DEVICE || memType == NVBUF_MEM_CUDA_PINNED || memType == NVBUF_MEM_CUDA_UNIFIED
        || memType == NVBUF_MEM_DEFAULT;
    return cuda;
}

}  // namespace

Capturer::Capturer() {
    stopWriter = false;
    writer = std::thread(&Capturer::writerLoop, this);
}

Capturer::~Capturer() {
    stop();
}

void Capturer::prepare(CaptureJob* job, NvDsFrameMeta* frame) {
    (void)job;
    (void)frame;
}

void Capturer::stop() {
    {
        std::lock_guard<std::mutex> lock(writerMutex);
        stopWriter = true;
    }
    writerCv.notify_all();
    if (writer.joinable()) {
        writer.join();
    }
}

std::string Capturer::eventPath(NvDsFrameMeta* frame, const char* key) const {
    std::string path;
    const char* line = collectorLine(frame);
    const std::string text = line != nullptr ? line : "";
    const std::string marker = std::string("\"") + key + "\":\"";
    const std::size_t start = text.find(marker);
    if (start != std::string::npos) {
        const std::size_t value = start + marker.size();
        const std::size_t end = text.find('"', value);
        if (end != std::string::npos) {
            path = text.substr(value, end - value);
        }
    }
    return path;
}

bool Capturer::shouldDump(NvDsFrameMeta* frame) const {
    const bool dump = frame != nullptr && !eventPath(frame, dumpKey()).empty();
    return dump;
}

void Capturer::packRgb(
    const std::uint8_t* src,
    unsigned int pitch,
    int width,
    int height,
    int channels,
    bool swapRb,
    std::vector<std::uint8_t>* rgb) {
    rgb->assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3, 0);
    for (int y = 0; y < height; y++) {
        const std::uint8_t* row = src + static_cast<std::size_t>(y) * pitch;
        for (int x = 0; x < width; x++) {
            const std::uint8_t* pixel = row + static_cast<std::size_t>(x) * static_cast<std::size_t>(channels);
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 3;
            std::uint8_t red = pixel[0];
            std::uint8_t blue = pixel[2];
            if (swapRb) {
                red = pixel[2];
                blue = pixel[0];
            }
            (*rgb)[offset] = red;
            (*rgb)[offset + 1] = pixel[1];
            (*rgb)[offset + 2] = blue;
        }
    }
}

bool Capturer::copyCuda(NvBufSurfaceParams* params, int channels, bool swapRb, std::vector<std::uint8_t>* rgb) {
    bool ok = false;
    const int width = static_cast<int>(params->width);
    const int height = static_cast<int>(params->height);
    const std::size_t rowBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    std::vector<std::uint8_t> packed;
    if (params->dataPtr != nullptr && rowBytes > 0 && rowBytes <= static_cast<std::size_t>(params->pitch)) {
        packed.resize(rowBytes * static_cast<std::size_t>(height));
        const cudaError_t err = cudaMemcpy2D(
            packed.data(), rowBytes, params->dataPtr, static_cast<std::size_t>(params->pitch), rowBytes, static_cast<std::size_t>(height), cudaMemcpyDefault);
        ok = err == cudaSuccess;
        if (ok) {
            packRgb(packed.data(), static_cast<unsigned int>(rowBytes), width, height, channels, swapRb, rgb);
        } else {
            g_warning("capturer cudaMemcpy2D failed: %s", cudaGetErrorString(err));
        }
    }
    return ok;
}

bool Capturer::copyMapped(
    NvBufSurface* surface,
    unsigned int batchId,
    NvBufSurfaceParams* params,
    int channels,
    bool swapRb,
    std::vector<std::uint8_t>* rgb) {
    bool ok = false;
    const int batch = static_cast<int>(batchId);
    if (NvBufSurfaceMap(surface, batch, -1, NVBUF_MAP_READ) == 0) {
        NvBufSurfaceSyncForCpu(surface, batch, -1);
        auto* src = static_cast<const std::uint8_t*>(params->mappedAddr.addr[0]);
        if (src != nullptr) {
            packRgb(src, params->pitch, static_cast<int>(params->width), static_cast<int>(params->height), channels, swapRb, rgb);
            ok = true;
        }
        NvBufSurfaceUnMap(surface, batch, -1);
    }
    return ok;
}

bool Capturer::copyRgb(NvBufSurface* surface, unsigned int batchId, std::vector<std::uint8_t>* rgb, int* width, int* height) {
    bool ok = false;
    int outWidth = 0;
    int outHeight = 0;
    rgb->clear();
    if (surface != nullptr && batchId < surface->numFilled) {
        NvBufSurfaceParams* params = &surface->surfaceList[batchId];
        const NvBufSurfaceColorFormat format = params->colorFormat;
        const bool packed = rgbFormat(format) || bgrFormat(format);
        const int channels = channelCount(format);
        const bool swapRb = bgrFormat(format);
        outWidth = static_cast<int>(params->width);
        outHeight = static_cast<int>(params->height);
        if (packed && outWidth > 0 && outHeight > 0 && cudaMemory(surface->memType)) {
            ok = copyCuda(params, channels, swapRb, rgb);
        }
        if (packed && outWidth > 0 && outHeight > 0 && !ok) {
            ok = copyMapped(surface, batchId, params, channels, swapRb, rgb);
        }
    }
    *width = outWidth;
    *height = outHeight;
    return ok;
}

void Capturer::enqueue(CaptureJob job) {
    {
        std::lock_guard<std::mutex> lock(writerMutex);
        writerQueue.push_back(std::move(job));
    }
    writerCv.notify_one();
}

void Capturer::writerLoop() {
    bool running = true;
    while (running) {
        CaptureJob job;
        bool hasJob = false;
        {
            std::unique_lock<std::mutex> lock(writerMutex);
            while (writerQueue.empty() && !stopWriter) {
                writerCv.wait(lock);
            }
            if (!writerQueue.empty()) {
                job = std::move(writerQueue.front());
                writerQueue.pop_front();
                hasJob = true;
            }
            running = hasJob || !stopWriter;
        }
        if (hasJob) {
            save(job);
        }
    }
}

bool Capturer::capture(NvBufSurface* surface, NvDsFrameMeta* frame) {
    bool ok = true;
    if (shouldDump(frame)) {
        CaptureJob job;
        job.rawPath = eventPath(frame, collector::KEY_RAW_PATH);
        job.visualizationPath = eventPath(frame, collector::KEY_VISUALIZATION_PATH);
        job.width = 0;
        job.height = 0;
        prepare(&job, frame);
        ok = copyRgb(surface, frame->batch_id, &job.rgb, &job.width, &job.height);
        if (ok) {
            enqueue(std::move(job));
        }
    }
    return ok;
}
