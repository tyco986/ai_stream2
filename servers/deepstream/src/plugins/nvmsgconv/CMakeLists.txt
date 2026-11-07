find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

function(add_msgconv_payload target definition engine_src output_name)
    add_library(${target} SHARED
        msgconv_payload.cpp
        ../dslogger/engine/log_engine.cpp
        ${engine_src}
    )
    target_compile_definitions(${target} PRIVATE ${definition})
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        /opt/nvidia/deepstream/deepstream/sources/includes
        /opt/nvidia/deepstream/deepstream/sources/libs/nvmsgconv
        ${GLIB_INCLUDE_DIRS}
    )
    target_compile_options(${target} PRIVATE -Wall -Werror ${GLIB_CFLAGS_OTHER})
    target_link_directories(${target} PRIVATE
        /opt/nvidia/deepstream/deepstream/lib
    )
    target_link_libraries(${target} PRIVATE
        nvds_meta
        spdlog::spdlog
        ${GLIB_LIBRARIES}
    )
    target_link_options(${target} PRIVATE
        -Wl,-no-undefined
        -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
    )
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output_name})
endfunction()

add_msgconv_payload(nvmsgconv_det MSGCONV_DET ../dslogger/engine/det_log_engine.cpp nvds_msgconv_det)
add_msgconv_payload(nvmsgconv_presence MSGCONV_PRESENCE ../dslogger/engine/presence_log_engine.cpp nvds_msgconv_presence)
add_msgconv_payload(nvmsgconv_stgcnpp MSGCONV_STGCNPP ../dslogger/engine/stgcnpp_log_engine.cpp nvds_msgconv_stgcnpp)
