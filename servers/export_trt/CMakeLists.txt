cmake_minimum_required(VERSION 3.16)
project(export_trt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(libzip REQUIRED)
find_package(Threads REQUIRED)

add_executable(export_trt
    main.cpp
    src/routes/health.cpp
    src/routes/types.cpp
    src/routes/exporter.cpp
    src/logger/trt_logger.cpp
    src/response/response.cpp
    src/services/exporter.cpp
)

target_include_directories(export_trt PRIVATE src)
target_link_libraries(export_trt PRIVATE nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp libzip::zip nvinfer nvonnxparser cudart ${CMAKE_DL_LIBS} Threads::Threads)
