find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)

add_library(dslogger SHARED
    engine/log_engine.cpp
    engine/det_log_engine.cpp
    engine/presence_log_engine.cpp
    engine/stgcnpp_log_engine.cpp
    gst_bridge/gst_logger_utils.cpp
    gst_bridge/gst_det_logger.cpp
    gst_bridge/gst_presence_logger.cpp
    gst_bridge/gst_stgcnpp_logger.cpp
    register_plugin.cpp
)

target_include_directories(dslogger PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(dslogger PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_compile_definitions(dslogger PRIVATE PACKAGE="dslogger")
target_link_directories(dslogger PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(dslogger PRIVATE
    nvdsgst_meta
    nvds_meta
    spdlog::spdlog
    ${GSTREAMER_LIBRARIES}
)
target_link_options(dslogger PRIVATE
    -Wl,-no-undefined
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
)
set_target_properties(dslogger PROPERTIES OUTPUT_NAME gstdslogger)
