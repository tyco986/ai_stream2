cmake_minimum_required(VERSION 3.16)

project(gsource_remove_guard)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

add_library(gsource_remove_guard SHARED gsource_remove_guard.cpp)
target_compile_features(gsource_remove_guard PRIVATE cxx_std_17)
target_include_directories(gsource_remove_guard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLIB_INCLUDE_DIRS}
)
target_compile_options(gsource_remove_guard PRIVATE -Wall -Werror ${GLIB_CFLAGS_OTHER})
target_link_libraries(gsource_remove_guard PRIVATE ${GLIB_LIBRARIES} dl)
target_link_options(gsource_remove_guard PRIVATE -Wl,-no-undefined)
