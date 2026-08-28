cmake_minimum_required(VERSION 3.16)

project(nvdsgst_capturer)

find_package(PNG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(nvdsgst_capturer SHARED
    png_writer.cpp
    capture_core.cpp
    gstnvrawcapturer.cpp
    gstnvviscapturer.cpp
    gstnvcapturer_plugin.cpp
)
target_compile_features(nvdsgst_capturer PRIVATE cxx_std_17)
target_include_directories(nvdsgst_capturer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../nveventcoder
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_capturer PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(nvdsgst_capturer PRIVATE PACKAGE="nvdsgst_capturer")
target_link_directories(nvdsgst_capturer PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-13.1/lib64/stubs
)
target_link_libraries(nvdsgst_capturer PRIVATE
    nvdsgst_meta
    nvds_meta
    nvbufsurface
    cuda
    PNG::PNG
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_capturer PRIVATE -Wl,-no-undefined -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib -Wl,-rpath-link,/usr/local/cuda-13.1/lib64/stubs)
