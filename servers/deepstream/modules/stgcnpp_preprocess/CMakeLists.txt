cmake_minimum_required(VERSION 3.16)

project(nvds_stgcnpp_preprocess)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

if(NOT DEFINED ENV{CUDA_VER} OR "$ENV{CUDA_VER}" STREQUAL "")
    set(CUDA_VER "13.1")
else()
    set(CUDA_VER "$ENV{CUDA_VER}")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(nvds_stgcnpp_preprocess SHARED nvdspreprocess_lib.cpp)
target_compile_features(nvds_stgcnpp_preprocess PRIVATE cxx_std_17)
target_link_options(nvds_stgcnpp_preprocess PRIVATE -Wl,-no-undefined)
target_include_directories(nvds_stgcnpp_preprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/sources/includes
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/include
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_link_directories(nvds_stgcnpp_preprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64
)
target_link_libraries(nvds_stgcnpp_preprocess PRIVATE
    nvdsgst_meta
    nvds_meta
    nvbufsurface
    nvbufsurftransform
    cudart
    cuda
    ${GSTREAMER_LIBRARIES}
)
