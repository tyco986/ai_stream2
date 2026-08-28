cmake_minimum_required(VERSION 3.16)

project(nvdsgst_fadedrawer)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(nvdsgst_fadedrawer SHARED
    fade_core.cpp
    seg_fade_engine.cpp
    pose_fade_engine.cpp
    stgcnpp_pose_fade_engine.cpp
    gstnvdetfadedrawer.cpp
    gstnvdetfadedrawerwithtracker.cpp
    gstnvsegfadedrawer.cpp
    gstnvsegfadedrawerwithtracker.cpp
    gstnvposefadedrawer.cpp
    gstnvposefadedrawerwithtracker.cpp
    gstnvstgcnppfadedrawerwithtracker.cpp
    gstnvfadedrawer_plugin.cpp
)
target_compile_features(nvdsgst_fadedrawer PRIVATE cxx_std_17)
target_include_directories(nvdsgst_fadedrawer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../nvbboxsnapshot
    ${CMAKE_CURRENT_SOURCE_DIR}/../nveventcoder
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_fadedrawer PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(nvdsgst_fadedrawer PRIVATE PACKAGE="nvdsgst_fadedrawer")
target_link_directories(nvdsgst_fadedrawer PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(nvdsgst_fadedrawer PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_fadedrawer PRIVATE -Wl,-no-undefined -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib)
