cmake_minimum_required(VERSION 3.16)

project(latency_probe)
find_package(nvds_service_maker REQUIRED PATHS /opt/nvidia/deepstream/deepstream/service-maker/cmake)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

add_library(latency_probe SHARED latency_probe.cpp)
target_include_directories(latency_probe PRIVATE
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)
target_link_directories(latency_probe PRIVATE /opt/nvidia/deepstream/deepstream/lib)
target_link_libraries(latency_probe PRIVATE
    nvds_service_maker
    nvds_service_maker_utils
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
    ${GLIB_LIBRARIES}
)
