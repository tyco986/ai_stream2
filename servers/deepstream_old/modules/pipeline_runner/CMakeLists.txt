cmake_minimum_required(VERSION 3.16)

project(pipeline_runner)

find_package(nvds_service_maker REQUIRED PATHS /opt/nvidia/deepstream/deepstream/service-maker/cmake)
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

if(NOT DEFINED ENV{CUDA_VER} OR "$ENV{CUDA_VER}" STREQUAL "")
    set(CUDA_VER "13.1")
else()
    set(CUDA_VER "$ENV{CUDA_VER}")
endif()

add_executable(pipeline_runner
    main.cpp
    pipeline_runner.cpp
    request_pads.cpp
)
target_compile_features(pipeline_runner PRIVATE cxx_std_17)
target_include_directories(pipeline_runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${YAML_CPP_INCLUDE_DIRS}
)
target_compile_options(pipeline_runner PRIVATE -Wall -Werror ${YAML_CPP_CFLAGS_OTHER})
target_link_directories(pipeline_runner PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64/stubs
    ${YAML_CPP_LIBRARY_DIRS}
)
target_link_libraries(pipeline_runner PRIVATE
    nvds_service_maker
    cuda
    ${YAML_CPP_LIBRARIES}
)
target_link_options(pipeline_runner PRIVATE
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
    -Wl,-rpath-link,/usr/local/cuda-${CUDA_VER}/lib64/stubs
)
