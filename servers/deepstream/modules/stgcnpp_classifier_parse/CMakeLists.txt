cmake_minimum_required(VERSION 3.16)

project(nvds_stgcnpp_classifier_parse)

if(NOT DEFINED ENV{CUDA_VER} OR "$ENV{CUDA_VER}" STREQUAL "")
    set(CUDA_VER "13.1")
else()
    set(CUDA_VER "$ENV{CUDA_VER}")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(nvds_stgcnpp_classifier_parse SHARED nvdsinfer_customclassifierparse.cpp)
target_compile_features(nvds_stgcnpp_classifier_parse PRIVATE cxx_std_17)
target_compile_options(nvds_stgcnpp_classifier_parse PRIVATE -Wall -Werror)
target_link_options(nvds_stgcnpp_classifier_parse PRIVATE -Wl,-no-undefined)
target_include_directories(nvds_stgcnpp_classifier_parse PRIVATE
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/cuda-${CUDA_VER}/include
)
target_link_directories(nvds_stgcnpp_classifier_parse PRIVATE
    /usr/local/cuda-${CUDA_VER}/lib64
)
target_link_libraries(nvds_stgcnpp_classifier_parse PRIVATE cudart)

