cmake_minimum_required(VERSION 3.16)
project(updator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(libzip REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

add_executable(updator
    main.cpp
    src/routes/health_route.cpp
    src/routes/logs_route.cpp
    src/routes/pack_route.cpp
    src/routes/update_route.cpp
    src/logger/request_logger.cpp
    src/response/response.cpp
    src/services/pack_service.cpp
    src/services/update_service.cpp
    src/services/logs_service.cpp
)

target_include_directories(updator PRIVATE src)
target_link_libraries(updator PRIVATE nlohmann_json::nlohmann_json libzip::zip spdlog::spdlog Threads::Threads)
