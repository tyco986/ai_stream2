find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_library(rtmposepreprocess SHARED
    engine/rect_expand.cpp
    nvdspreprocess_lib.cpp
)

target_include_directories(rtmposepreprocess PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(rtmposepreprocess PRIVATE -Wall -Werror)
target_link_directories(rtmposepreprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(rtmposepreprocess PRIVATE
    nvbufsurface
    nvbufsurftransform
)
target_link_options(rtmposepreprocess PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(rtmposepreprocess PROPERTIES OUTPUT_NAME nvds_rtmpose_preprocess)
