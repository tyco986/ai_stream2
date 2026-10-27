cmake_minimum_required(VERSION 3.16)
project(export_trt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(libzip REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

add_executable(export_trt
    main.cpp
    src/routes/health_route.cpp
    src/routes/types_route.cpp
    src/routes/exporter_route.cpp
    src/routes/history_route.cpp
    src/logger/trt_logger.cpp
    src/logger/request_logger.cpp
    src/response/response.cpp
    src/services/exporter_service.cpp
    src/services/history_service.cpp
)

target_include_directories(export_trt PRIVATE src /usr/local/cuda/include)
target_link_directories(export_trt PRIVATE /usr/local/cuda/lib64)
target_link_libraries(export_trt PRIVATE nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp libzip::zip spdlog::spdlog nvinfer nvonnxparser cudart ${CMAKE_DL_LIBS} Threads::Threads)
