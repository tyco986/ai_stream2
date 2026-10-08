cmake_minimum_required(VERSION 3.16)
project(ffmpeg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

add_executable(ffmpeg
    main.cpp
    src/routes/rtsp.cpp
    src/services/rtsp.cpp
    src/response/response.cpp
)

target_include_directories(ffmpeg PRIVATE src)
target_link_libraries(ffmpeg PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
