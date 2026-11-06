find_package(PkgConfig REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(capturer SHARED
    engine/capturer.cpp
    engine/raw_capturer.cpp
    engine/vis_capturer.cpp
    engine/png_writer.cpp
    engine/jpeg_writer.cpp
    gst_bridge/gst_capturer_utils.cpp
    gst_bridge/gst_raw_capturer.cpp
    gst_bridge/gst_vis_capturer.cpp
    register_plugin.cpp
)

target_include_directories(capturer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/cuda-13.1/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(capturer PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(capturer PRIVATE PACKAGE="capturer")
target_link_directories(capturer PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-13.1/lib64
)
target_link_libraries(capturer PRIVATE
    nvdsgst_meta
    nvds_meta
    nvbufsurface
    cudart
    PNG::PNG
    JPEG::JPEG
    ${GSTREAMER_LIBRARIES}
)
target_link_options(capturer PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
    -Wl,-rpath,/usr/local/cuda-13.1/lib64
)
set_target_properties(capturer PROPERTIES OUTPUT_NAME gstcapturer)
