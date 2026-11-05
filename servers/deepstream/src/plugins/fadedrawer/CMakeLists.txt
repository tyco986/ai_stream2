find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(fadedrawer SHARED
    det_fade_engine.cpp
    det_fade_engine_with_tracker.cpp
    seg_fade_engine.cpp
    seg_fade_engine_with_tracker.cpp
    pose_fade_engine.cpp
    pose_fade_engine_with_tracker.cpp
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
