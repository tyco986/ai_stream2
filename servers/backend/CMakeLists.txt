cmake_minimum_required(VERSION 3.16)
project(backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(backend
  main.cpp
  src/auth/auth.cpp
  src/db/database.cpp
  src/controllers/health.cpp
  src/controllers/log.cpp
  src/controllers/stream_group.cpp
  src/controllers/stream.cpp
  src/controllers/stream_batch.cpp
  src/controllers/proxy_routes.cpp
  src/proxy/proxy.cpp
  src/response/response.cpp
  src/searcher/searcher.cpp
  src/mediamtx/mediamtx.cpp
  src/ffmpeg/ffmpeg.cpp
  src/services/stream_group.cpp
  src/services/stream.cpp
  src/log/base_logger.cpp
  src/log/stream_logger.cpp
  src/log/stream_group_logger.cpp)
target_include_directories(backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(backend PRIVATE Drogon::Drogon spdlog::spdlog)
