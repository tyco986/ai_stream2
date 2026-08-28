#include "png_writer.hpp"

#include <csetjmp>
#include <cstdio>

#include <png.h>
#include <glib.h>
#include <glib/gstdio.h>

PngWriter::PngWriter() {}

bool PngWriter::write_rgb(
    const std::string &path,
    const std::vector<uint8_t> &rgb,
    int width,
    int height)
{
  bool ok = false;
  FILE *file = nullptr;
  png_structp png = nullptr;
  png_infop info = nullptr;
  std::vector<png_bytep> rows;
  guint64 stride = 0;
  guint64 expected = 0;
  if (width > 0 && height > 0) {
    stride = static_cast<guint64>(width) * 3;
    expected = stride * static_cast<guint64>(height);
  }
  if (expected > 0 && rgb.size() >= expected) {
    gchar *parent = g_path_get_dirname(path.c_str());
    g_mkdir_with_parents(parent, 0755);
    g_free(parent);
    file = fopen(path.c_str(), "wb");
  }
  if (file != nullptr) {
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png != nullptr) {
      info = png_create_info_struct(png);
    }
  }
  if (png != nullptr && info != nullptr && setjmp(png_jmpbuf(png)) == 0) {
    png_init_io(png, file);
    png_set_IHDR(
        png,
        info,
        static_cast<png_uint_32>(width),
        static_cast<png_uint_32>(height),
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    rows.resize(static_cast<size_t>(height));
    for (int y = 0; y < height; y++) {
      rows[static_cast<size_t>(y)] =
          const_cast<png_bytep>(rgb.data() + static_cast<size_t>(y) * static_cast<size_t>(stride));
    }
    png_write_image(png, rows.data());
    png_write_end(png, nullptr);
    ok = true;
  }
  if (png != nullptr) {
    png_destroy_write_struct(&png, info != nullptr ? &info : nullptr);
  }
  if (file != nullptr) {
    fclose(file);
  }
  if (!ok && !path.empty()) {
    g_remove(path.c_str());
  }
  return ok;
}
