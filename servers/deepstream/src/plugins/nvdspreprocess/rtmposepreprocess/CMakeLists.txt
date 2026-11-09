find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

set(NVDSPREPROCESS_SAMPLE /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/nvdspreprocess_lib)

enable_language(CUDA)

add_library(rtmpose_nvds_tensor STATIC
    ${NVDSPREPROCESS_SAMPLE}/nvdspreprocess_impl.cpp
    ${NVDSPREPROCESS_SAMPLE}/nvdspreprocess_conversion.cu
)
set_target_properties(rtmpose_nvds_tensor PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CUDA_ARCHITECTURES native
)
target_include_directories(rtmpose_nvds_tensor PRIVATE
    ${NVDSPREPROCESS_SAMPLE}
    /opt/nvidia/deepstream/deepstream/sources/includes
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)

add_library(rtmposepreprocess SHARED
    engine/rect_expand.cpp
    nvdspreprocess_lib.cpp
)

target_include_directories(rtmposepreprocess PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${NVDSPREPROCESS_SAMPLE}
    /opt/nvidia/deepstream/deepstream/sources/includes
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(rtmposepreprocess PRIVATE -Wall -Werror)
target_link_directories(rtmposepreprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64
)
target_link_libraries(rtmposepreprocess PRIVATE
    rtmpose_nvds_tensor
    nvbufsurface
    nvbufsurftransform
    cudart
    cuda
    dl
)
target_link_options(rtmposepreprocess PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
    -Wl,-rpath,/usr/local/cuda-${CUDA_VER}/lib64
)
set_target_properties(rtmposepreprocess PROPERTIES OUTPUT_NAME nvds_rtmpose_preprocess)
