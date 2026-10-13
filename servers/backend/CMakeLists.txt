cmake_minimum_required(VERSION 3.16)
project(backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(backend
  main.cpp
  src/auth/auth.cpp
  src/db/database.cpp
  src/controllers/health_controller.cpp
  src/controllers/log_controller.cpp
  src/controllers/stream_group_controller.cpp
  src/controllers/stream_controller.cpp
  src/controllers/stream_batch_controller.cpp
  src/controllers/model_controller.cpp
  src/controllers/model_batch_controller.cpp
  src/controllers/proxy_routes_controller.cpp
  src/proxy/proxy.cpp
  src/response/response.cpp
  src/searcher/searcher.cpp
  src/mediamtx/mediamtx.cpp
  src/ffmpeg/ffmpeg.cpp
  src/services/stream_group_service.cpp
  src/services/stream_service.cpp
  src/services/model_service.cpp
  src/log/base_logger.cpp
  src/log/stream_logger.cpp
  src/log/stream_group_logger.cpp
  src/log/model_logger.cpp)
target_include_directories(backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(backend PRIVATE Drogon::Drogon spdlog::spdlog)
