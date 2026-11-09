#include "plugins/capturer/engine/vis_capturer.hpp"

#include "constants/capturer_const.hpp"

#include <cstdio>
#include <format>
#include <sstream>

#include <glib.h>
#include <glib/gstdio.h>

VisCapturer::VisCapturer(const char* captureCodes, const char* labelTask) : Capturer(captureCodes) {
    const std::string next = (labelTask != nullptr && labelTask[0] != '\0') ? labelTask : capturer::DEFAULT_LABEL_TASK;
    this->labelTask = capturer::DEFAULT_LABEL_TASK;
    if (next == capturer::DEFAULT_LABEL_TASK || next == capturer::LABEL_TASK_SEG) {
        this->labelTask = next;
    }
}

VisCapturer::~VisCapturer() {
    stop();
}

void VisCapturer::prepare(CaptureJob* job, NvDsFrameMeta* frame) {
    job->boxes.clear();
    NvDsMetaList* item = frame != nullptr ? frame->obj_meta_list : nullptr;
    for (; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            CaptureBox box;
            box.left = object->rect_params.left;
            box.top = object->rect_params.top;
            box.width = object->rect_params.width;
            box.height = object->rect_params.height;
            box.classId = object->class_id;
            box.label = object->obj_label[0] != '\0' ? object->obj_label : std::to_string(object->class_id);
            job->boxes.push_back(box);
        }
    }
}

std::string VisCapturer::jsonEscape(const std::string& text) const {
    std::string escaped;
    escaped.reserve(text.size());
    for (char item : text) {
        if (item == '\\' || item == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(item);
    }
    return escaped;
}

bool VisCapturer::writeYolo(const std::string& path, const std::vector<CaptureBox>& boxes, int imageWidth, int imageHeight) {
    bool ok = false;
    gchar* parent = g_path_get_dirname(path.c_str());
    g_mkdir_with_parents(parent, 0755);
    g_free(parent);
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file != nullptr) {
        ok = true;
        if (imageWidth > 0 && imageHeight > 0) {
            for (const CaptureBox& box : boxes) {
                const float centerX = (box.left + box.width / 2.0f) / static_cast<float>(imageWidth);
                const float centerY = (box.top + box.height / 2.0f) / static_cast<float>(imageHeight);
                const float boxWidth = box.width / static_cast<float>(imageWidth);
                const float boxHeight = box.height / static_cast<float>(imageHeight);
                if (std::fprintf(file, "%d %.6f %.6f %.6f %.6f\n", box.classId, centerX, centerY, boxWidth, boxHeight) < 0) {
                    ok = false;
                }
            }
        }
        if (std::fclose(file) != 0) {
            ok = false;
        }
    }
    return ok;
}

bool VisCapturer::writeLabelme(
    const std::string& path,
    const std::vector<CaptureBox>& boxes,
    int imageWidth,
    int imageHeight,
    const std::string& imagePath) {
    bool ok = false;
    gchar* parent = g_path_get_dirname(path.c_str());
    g_mkdir_with_parents(parent, 0755);
    g_free(parent);
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file != nullptr) {
        std::ostringstream body;
        body << "{\n";
        body << "  \"version\": \"5.0.1\",\n";
        body << "  \"flags\": {},\n";
        body << "  \"shapes\": [\n";
        for (std::size_t index = 0; index < boxes.size(); index++) {
            const CaptureBox& box = boxes[index];
            const float right = box.left + box.width;
            const float bottom = box.top + box.height;
            body << "    {\n";
            body << "      \"label\": \"" << jsonEscape(box.label) << "\",\n";
            body << "      \"points\": [[" << box.left << ", " << box.top << "], [" << right << ", " << bottom << "]],\n";
            body << "      \"group_id\": null,\n";
            body << "      \"shape_type\": \"rectangle\",\n";
            body << "      \"flags\": {}\n";
            body << "    }";
            if (index + 1 < boxes.size()) {
                body << ",";
            }
            body << "\n";
        }
        body << "  ],\n";
        body << "  \"imagePath\": \"" << jsonEscape(imagePath) << "\",\n";
        body << "  \"imageData\": null,\n";
        body << "  \"imageHeight\": " << imageHeight << ",\n";
        body << "  \"imageWidth\": " << imageWidth << "\n";
        body << "}\n";
        const std::string text = body.str();
        ok = std::fwrite(text.data(), 1, text.size(), file) == text.size();
        if (std::fclose(file) != 0) {
            ok = false;
        }
    }
    return ok;
}

bool VisCapturer::writeLabels(const CaptureJob& job) {
    const std::string stem = std::format("raw_{:03d}_{:08d}", job.padIndex, job.captureId);
    const std::string yoloPath = std::format("{}/labels/{}.txt", outputRoot(), stem);
    const std::string jsonPath = std::format("{}/labelme/{}.json", outputRoot(), stem);
    const std::string imagePath = std::format("../images/{}.png", stem);
    const bool yoloOk = writeYolo(yoloPath, job.boxes, job.width, job.height);
    const bool jsonOk = writeLabelme(jsonPath, job.boxes, job.width, job.height, imagePath);
    const bool ok = yoloOk && jsonOk;
    return ok;
}

bool VisCapturer::save(const CaptureJob& job) {
    const std::string path = std::format("{}/vis/vis_{:03d}_{:08d}.jpg", outputRoot(), job.padIndex, job.captureId);
    const bool imageOk = jpeg.write(path, job.rgb, job.width, job.height);
    const bool labelsOk = writeLabels(job);
    const bool ok = imageOk && labelsOk;
    if (!ok) {
        g_warning("viscapturer dump failed path=%s jpg=%d labels=%d task=%s", path.c_str(), static_cast<int>(imageOk), static_cast<int>(labelsOk), labelTask.c_str());
    }
    return ok;
}
