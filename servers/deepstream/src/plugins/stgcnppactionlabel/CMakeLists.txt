find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(stgcnppactionlabel SHARED
    engine/stgcnpp_action_label.cpp
    gst_bridge/gst_stgcnpp_action_label.cpp
    register_plugin.cpp
)

target_include_directories(stgcnppactionlabel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(stgcnppactionlabel PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(stgcnppactionlabel PRIVATE PACKAGE="stgcnppactionlabel")
target_link_directories(stgcnppactionlabel PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(stgcnppactionlabel PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(stgcnppactionlabel PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(stgcnppactionlabel PROPERTIES OUTPUT_NAME gststgcnppactionlabel)
