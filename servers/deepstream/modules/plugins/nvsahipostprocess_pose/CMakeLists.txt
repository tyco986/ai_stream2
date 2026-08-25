cmake_minimum_required(VERSION 3.16)

project(nvdsgst_sahipostprocess_pose)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)
find_package(OpenMP REQUIRED)

add_library(nvdsgst_sahipostprocess_pose SHARED gstnvsahipostprocess_pose.cpp)
target_compile_features(nvdsgst_sahipostprocess_pose PRIVATE cxx_std_17)
target_include_directories(nvdsgst_sahipostprocess_pose PRIVATE
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${GSTREAMER_INCLUDE_DIRS}
)
target_compile_options(nvdsgst_sahipostprocess_pose PRIVATE -Wall -Werror ${GSTREAMER_CFLAGS_OTHER})
target_link_directories(nvdsgst_sahipostprocess_pose PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
)
target_link_libraries(nvdsgst_sahipostprocess_pose PRIVATE
    nvdsgst_meta
    nvds_meta
    OpenMP::OpenMP_CXX
    ${GSTREAMER_LIBRARIES}
)
target_link_options(nvdsgst_sahipostprocess_pose PRIVATE -Wl,-no-undefined -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib)
