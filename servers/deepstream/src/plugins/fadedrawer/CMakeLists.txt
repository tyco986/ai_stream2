find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(fadedrawer SHARED
    engine/det_fade_engine.cpp
    engine/det_fade_engine_with_tracker.cpp
    engine/seg_fade_engine.cpp
    engine/seg_fade_engine_with_tracker.cpp
    engine/pose_drawer.cpp
    engine/seg_mask.cpp
    engine/pose_fade_engine.cpp
    engine/pose_fade_engine_with_tracker.cpp
    engine/stgcnpp_fade_engine_with_tracker.cpp
    gst_bridge/gst_fade_drawer_utils.cpp
    gst_bridge/gst_det_fade_drawer.cpp
    gst_bridge/gst_det_fade_drawer_with_tracker.cpp
    gst_bridge/gst_seg_fade_drawer.cpp
    gst_bridge/gst_seg_fade_drawer_with_tracker.cpp
    gst_bridge/gst_pose_fade_drawer.cpp
    gst_bridge/gst_pose_fade_drawer_with_tracker.cpp
    gst_bridge/gst_stgcnpp_fade_drawer_with_tracker.cpp
    register_plugin.cpp
)

target_include_directories(fadedrawer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(fadedrawer PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(fadedrawer PRIVATE PACKAGE="fadedrawer")
target_link_directories(fadedrawer PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(fadedrawer PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(fadedrawer PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(fadedrawer PROPERTIES OUTPUT_NAME gstfadedrawer)
