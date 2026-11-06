#include "plugins/capturer/engine/jpeg_writer.hpp"

#include "constants/capturer_const.hpp"

#include <csetjmp>
#include <cstdio>

#include <glib.h>
#include <glib/gstdio.h>
#include <jpeglib.h>

namespace {

struct JpegError {
    jpeg_error_mgr pub;
    jmp_buf jump;
};

void jpegErrorExit(j_common_ptr info) {
    auto* error = reinterpret_cast<JpegError*>(info->err);
    std::longjmp(error->jump, 1);
}

}  // namespace

bool JpegWriter::write(const std::string& path, const std::vector<std::uint8_t>& rgb, int width, int height) {
    bool ok = false;
    FILE* file = nullptr;
    JpegError error;
    jpeg_compress_struct info;
    const unsigned long expected = (width > 0 && height > 0) ? static_cast<unsigned long>(width) * static_cast<unsigned long>(height) * 3 : 0;
    if (expected > 0 && rgb.size() >= expected) {
        gchar* parent = g_path_get_dirname(path.c_str());
        g_mkdir_with_parents(parent, 0755);
        g_free(parent);
        file = std::fopen(path.c_str(), "wb");
    }
    volatile bool created = false;
    if (file != nullptr) {
        info.err = jpeg_std_error(&error.pub);
        error.pub.error_exit = jpegErrorExit;
        if (setjmp(error.jump) == 0) {
            jpeg_create_compress(&info);
            created = true;
            jpeg_stdio_dest(&info, file);
            info.image_width = static_cast<JDIMENSION>(width);
            info.image_height = static_cast<JDIMENSION>(height);
            info.input_components = 3;
            info.in_color_space = JCS_RGB;
            jpeg_set_defaults(&info);
            jpeg_set_quality(&info, capturer::JPEG_QUALITY, TRUE);
            jpeg_start_compress(&info, TRUE);
            while (info.next_scanline < info.image_height) {
                JSAMPROW row = const_cast<JSAMPROW>(rgb.data() + static_cast<std::size_t>(info.next_scanline) * static_cast<std::size_t>(width) * 3);
                jpeg_write_scanlines(&info, &row, 1);
            }
            jpeg_finish_compress(&info);
            ok = true;
        }
        if (created) {
            jpeg_destroy_compress(&info);
        }
        std::fclose(file);
    }
    if (!ok && !path.empty()) {
        g_remove(path.c_str());
    }
    return ok;
}
