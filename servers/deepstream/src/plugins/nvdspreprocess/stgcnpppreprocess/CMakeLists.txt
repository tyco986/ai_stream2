find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_library(stgcnpppreprocess SHARED
    engine/stgcnpp_preprocess.cpp
    engine/stgcnpp_ready_meta.cpp
    nvdspreprocess_lib.cpp
)

target_include_directories(stgcnpppreprocess PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    /opt/nvidia/deepstream/deepstream/sources/gst-plugins/gst-nvdspreprocess/include
    /usr/local/cuda-${CUDA_VER}/include
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(stgcnpppreprocess PRIVATE -Wall -Werror)
target_link_directories(stgcnpppreprocess PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64
    ${GSTREAMER_LIBRARY_DIRS}
)
target_link_libraries(stgcnpppreprocess PRIVATE
    cudart
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(stgcnpppreprocess PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
    -Wl,-rpath,/usr/local/cuda-${CUDA_VER}/lib64
)
set_target_properties(stgcnpppreprocess PROPERTIES OUTPUT_NAME nvds_stgcnpp_preprocess)
