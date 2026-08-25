cmake_minimum_required(VERSION 3.16)

project(nvdsgst_logger)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(nvdsgst_logger SHARED
    det_log_engine.cpp
    gstnvdetlogger.cpp
    gstnvlogger_plugin.cpp
)
target_compile_features(nvdsgst_logger PRIVATE cxx_std_17)
target_include_directories(nvdsgst_logger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_logger PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_link_directories(nvdsgst_logger PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(nvdsgst_logger PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_logger PRIVATE -Wl,-no-undefined -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib)
