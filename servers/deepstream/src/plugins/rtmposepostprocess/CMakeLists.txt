find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(rtmposepostprocess SHARED
    engine/rtmpose_postprocess.cpp
    gst_bridge/gst_rtmpose_postprocess.cpp
    register_plugin.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/nvdspreprocess/rtmposepreprocess/engine/rect_expand.cpp
)

target_include_directories(rtmposepostprocess PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(rtmposepostprocess PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(rtmposepostprocess PRIVATE PACKAGE="rtmposepostprocess")
target_link_directories(rtmposepostprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64
    /usr/local/cuda-${CUDA_VER}/lib64/stubs
)
target_link_libraries(rtmposepostprocess PRIVATE
    nvdsgst_meta
    nvds_meta
    cudart
    ${GSTREAMER_LIBRARIES}
)
target_link_options(rtmposepostprocess PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(rtmposepostprocess PROPERTIES OUTPUT_NAME gstrtmposepostprocess)
