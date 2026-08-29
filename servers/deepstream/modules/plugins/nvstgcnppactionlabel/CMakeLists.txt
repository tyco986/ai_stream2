cmake_minimum_required(VERSION 3.16)

project(nvdsgst_stgcnppactionlabel)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(nvdsgst_stgcnppactionlabel SHARED
    action_label_engine.cpp
    nvds_stgcnpp_action_meta.cpp
    gstnvstgcnppactionlabel.cpp
    gstnvstgcnppactionlabel_plugin.cpp
)
target_compile_features(nvdsgst_stgcnppactionlabel PRIVATE cxx_std_17)
target_include_directories(nvdsgst_stgcnppactionlabel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../nvdspreprocess/stgcnpp_preprocess
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_stgcnppactionlabel PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(nvdsgst_stgcnppactionlabel PRIVATE PACKAGE="nvdsgst_stgcnppactionlabel")
target_link_directories(nvdsgst_stgcnppactionlabel PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(nvdsgst_stgcnppactionlabel PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_stgcnppactionlabel PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
