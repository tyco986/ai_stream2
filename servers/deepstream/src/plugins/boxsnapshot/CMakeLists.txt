find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(boxsnapshot SHARED
    engine/boxsnapshot.cpp
    gst_bridge/gst_boxsnapshot.cpp
    register_plugin.cpp
)

target_include_directories(boxsnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(boxsnapshot PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(boxsnapshot PRIVATE PACKAGE="boxsnapshot")
target_link_directories(boxsnapshot PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(boxsnapshot PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(boxsnapshot PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(boxsnapshot PROPERTIES OUTPUT_NAME gstboxsnapshot)
