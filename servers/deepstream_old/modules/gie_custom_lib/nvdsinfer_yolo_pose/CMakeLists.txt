cmake_minimum_required(VERSION 3.16)

project(nvds_infer_yolo_pose)

if(NOT DEFINED ENV{CUDA_VER} OR "$ENV{CUDA_VER}" STREQUAL "")
    set(CUDA_VER "13.1")
else()
    set(CUDA_VER "$ENV{CUDA_VER}")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(nvds_infer_yolo_pose SHARED nvdsinfer_yolo_pose.cpp)
set_target_properties(nvds_infer_yolo_pose PROPERTIES OUTPUT_NAME nvds_infer_yolo_pose)
target_compile_features(nvds_infer_yolo_pose PRIVATE cxx_std_17)
target_compile_options(nvds_infer_yolo_pose PRIVATE -Wall -Werror)
target_link_options(nvds_infer_yolo_pose PRIVATE -Wl,-no-undefined)
target_include_directories(nvds_infer_yolo_pose PRIVATE
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/cuda-${CUDA_VER}/include
)
target_link_directories(nvds_infer_yolo_pose PRIVATE
    /usr/local/cuda-${CUDA_VER}/lib64
)
target_link_libraries(nvds_infer_yolo_pose PRIVATE cudart)
