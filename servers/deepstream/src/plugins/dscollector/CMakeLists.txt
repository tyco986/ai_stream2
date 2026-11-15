find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(dscollector SHARED
    engine/collector_engine.cpp
    engine/presence_collector_engine.cpp
    engine/stgcnpp_collector_engine.cpp
    gst_bridge/gst_collector_utils.cpp
    gst_bridge/gst_det_collector.cpp
    gst_bridge/gst_presence_collector.cpp
    gst_bridge/gst_stgcnpp_collector.cpp
    register_plugin.cpp
)

target_include_directories(dscollector PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(dscollector PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(dscollector PRIVATE PACKAGE="dscollector")
target_link_directories(dscollector PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(dscollector PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(dscollector PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(dscollector PROPERTIES OUTPUT_NAME gstdscollector)
