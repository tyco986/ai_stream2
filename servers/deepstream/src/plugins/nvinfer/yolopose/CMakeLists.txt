add_library(nvinferyolopose SHARED
    engine/yolo_pose_parser.cpp
    nvinfer_lib.cpp
)

target_include_directories(nvinferyolopose PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/cuda-${CUDA_VER}/include
)
target_compile_options(nvinferyolopose PRIVATE -Wall -Werror)
target_link_options(nvinferyolopose PRIVATE -Wl,-no-undefined)
set_target_properties(nvinferyolopose PROPERTIES OUTPUT_NAME nvinfer_yolo_pose)
