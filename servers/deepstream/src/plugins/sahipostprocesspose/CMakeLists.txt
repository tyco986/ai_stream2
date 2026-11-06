find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(sahipostprocesspose SHARED
    engine/spatial_grid.cpp
    engine/sahi_pose_nms.cpp
    gst_bridge/gst_sahi_postprocess_pose.cpp
    register_plugin.cpp
)

target_include_directories(sahipostprocesspose PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(sahipostprocesspose PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(sahipostprocesspose PRIVATE PACKAGE="nvsahipostprocesspose")
target_link_directories(sahipostprocesspose PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(sahipostprocesspose PRIVATE
    nvdsgst_meta
    nvds_meta
    ${GSTREAMER_LIBRARIES}
)
target_link_options(sahipostprocesspose PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(sahipostprocesspose PROPERTIES OUTPUT_NAME gstnvsahipostprocesspose)
