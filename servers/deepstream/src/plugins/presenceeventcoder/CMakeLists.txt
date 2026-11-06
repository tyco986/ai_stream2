find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(presenceeventcoder SHARED
    engine/presence_event_coder.cpp
    gst_bridge/gst_presence_event_coder.cpp
    register_plugin.cpp
)

target_include_directories(presenceeventcoder PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(presenceeventcoder PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(presenceeventcoder PRIVATE PACKAGE="presenceeventcoder")
target_link_directories(presenceeventcoder PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(presenceeventcoder PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(presenceeventcoder PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(presenceeventcoder PROPERTIES OUTPUT_NAME gstpresenceeventcoder)
