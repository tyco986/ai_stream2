cmake_minimum_required(VERSION 3.16)

project(nvds_rtmpose_preprocess LANGUAGES CXX CUDA)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

if(NOT DEFINED ENV{CUDA_VER} OR "$ENV{CUDA_VER}" STREQUAL "")
    set(CUDA_VER "13.1")
else()
    set(CUDA_VER "$ENV{CUDA_VER}")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(nvds_rtmpose_preprocess SHARED nvdspreprocess_lib.cpp rgba_to_nchw.cu)
target_compile_features(nvds_rtmpose_preprocess PRIVATE cxx_std_17)
set_target_properties(nvds_rtmpose_preprocess PROPERTIES CUDA_ARCHITECTURES "75;80;86;89;90")
target_link_options(nvds_rtmpose_preprocess PRIVATE -Wl,-no-undefined)
target_include_directories(nvds_rtmpose_preprocess PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
set_source_files_properties(nvdspreprocess_lib.cpp PROPERTIES LANGUAGE CXX)
target_link_directories(nvds_rtmpose_preprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64
    /usr/local/cuda-${CUDA_VER}/lib64/stubs
)
target_link_libraries(nvds_rtmpose_preprocess PRIVATE
    nvdsgst_meta
    nvds_meta
    nvbufsurface
    nvbufsurftransform
    cudart
    cuda
    ${GSTREAMER_LIBRARIES}
)
