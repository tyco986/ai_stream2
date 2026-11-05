cmake_minimum_required(VERSION 3.16)
project(deepstream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CUDA_VER "13.1")

find_package(nvds_service_maker REQUIRED PATHS /opt/nvidia/deepstream/deepstream/service-maker/cmake)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
find_package(libzip REQUIRED)
find_package(spdlog REQUIRED)

add_executable(deepstream
    main.cpp
    src/routes/health_route.cpp
    src/routes/pipeline_route.cpp
    src/routes/logs_route.cpp
    src/logger/request_logger.cpp
    src/response/response.cpp
    src/services/pipeline_service.cpp
    src/services/logs_service.cpp
    src/pipeline_runner/pipeline_runner.cpp
)

target_include_directories(deepstream PRIVATE
    src
    ${YAML_CPP_INCLUDE_DIRS}
)
target_compile_definitions(deepstream PRIVATE CPPHTTPLIB_NO_OPENSSL)
target_compile_options(deepstream PRIVATE -Wall -Werror ${YAML_CPP_CFLAGS_OTHER})
target_link_directories(deepstream PRIVATE
    /opt/nvidia/deepstream/deepstream/lib
    /usr/local/cuda-${CUDA_VER}/lib64/stubs
    ${YAML_CPP_LIBRARY_DIRS}
)
target_link_libraries(deepstream PRIVATE
    nvds_service_maker
    cuda
    nlohmann_json::nlohmann_json
    libzip::zip
    spdlog::spdlog
    ${YAML_CPP_LIBRARIES}
    Threads::Threads
)
target_link_options(deepstream PRIVATE
    -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib
    -Wl,-rpath-link,/usr/local/cuda-${CUDA_VER}/lib64/stubs
)

add_subdirectory(src/plugins/boxsnapshot)
add_subdirectory(src/plugins/fadedrawer)
