cmake_minimum_required(VERSION 3.16)

project(nvdsgst_rtmposepostprocess)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

if(NOT DEFINED ENV{CUDA_VER} OR "$ENV{CUDA_VER}" STREQUAL "")
    set(CUDA_VER "13.1")
else()
    set(CUDA_VER "$ENV{CUDA_VER}")
endif()

add_library(nvdsgst_rtmposepostprocess SHARED
    gstnvrtmposepostprocess.cpp
    gstnvrtmposepostprocess_plugin.cpp
)
target_compile_features(nvdsgst_rtmposepostprocess PRIVATE cxx_std_17)
target_include_directories(nvdsgst_rtmposepostprocess PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../nvdspreprocess/rtmpose_preprocess
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_rtmposepostprocess PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(nvdsgst_rtmposepostprocess PRIVATE PACKAGE="nvdsgst_rtmposepostprocess")
target_link_directories(nvdsgst_rtmposepostprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64
    /usr/local/cuda-${CUDA_VER}/lib64/stubs
)
target_link_libraries(nvdsgst_rtmposepostprocess PRIVATE
    nvdsgst_meta
    nvds_meta
    cudart
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_rtmposepostprocess PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
