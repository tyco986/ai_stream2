cmake_minimum_required(VERSION 3.16)

project(nvdsgst_eventcoder)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(nvdsgst_eventcoder SHARED
    nvds_presence_event_meta.cpp
    presence_event_coder.cpp
    gstnvpresencecoder.cpp
    gstnvobjectcoder.cpp
    gstnveventcoder_plugin.cpp
)
target_compile_features(nvdsgst_eventcoder PRIVATE cxx_std_17)
target_include_directories(nvdsgst_eventcoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_eventcoder PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(nvdsgst_eventcoder PRIVATE PACKAGE="nvdsgst_eventcoder")
target_link_directories(nvdsgst_eventcoder PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(nvdsgst_eventcoder PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_eventcoder PRIVATE -Wl,-no-undefined -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib)
