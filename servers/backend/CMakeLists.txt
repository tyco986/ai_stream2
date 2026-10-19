cmake_minimum_required(VERSION 3.16)
project(backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(libzip REQUIRED)

find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka++)

add_executable(backend
  main.cpp
  src/auth/auth.cpp
  src/db/database.cpp
  src/controllers/health_controller.cpp
  src/controllers/log_controller.cpp
  src/controllers/stream_group_controller.cpp
  src/controllers/user_group_controller.cpp
  src/controllers/user_controller.cpp
  src/controllers/user_batch_controller.cpp
  src/controllers/stream_controller.cpp
  src/controllers/stream_batch_controller.cpp
  src/controllers/model_controller.cpp
  src/controllers/pipeline_controller.cpp
  src/controllers/pipeline_batch_controller.cpp
  src/controllers/pipeline_gie_controller.cpp
  src/controllers/pipeline_gie_batch_controller.cpp
  src/controllers/pipeline_analyzer_controller.cpp
  src/controllers/pipeline_analyzer_batch_controller.cpp
  src/controllers/model_batch_controller.cpp
  src/controllers/proxy_routes_controller.cpp
  src/controllers/server_controller.cpp
  src/controllers/preview_controller.cpp
  src/controllers/event_controller.cpp
  src/controllers/notice_controller.cpp
  src/proxy/proxy.cpp
  src/response/response.cpp
  src/searcher/searcher.cpp
  src/mediamtx/mediamtx.cpp
  src/ffmpeg/ffmpeg.cpp
  src/kafka/kafka.cpp
  src/services/stream_group_service.cpp
  src/services/user_group_service.cpp
  src/services/user_service.cpp
  src/services/stream_service.cpp
  src/services/model_service.cpp
  src/services/pipeline_service.cpp
  src/services/pipeline_gie_service.cpp
  src/services/pipeline_analyzer_service.cpp
  src/services/server_service.cpp
  src/services/preview_service.cpp
  src/services/event_service.cpp
  src/services/notice_service.cpp
  src/log/base_logger.cpp
  src/log/stream_logger.cpp
  src/log/stream_group_logger.cpp
  src/log/user_group_logger.cpp
  src/log/user_logger.cpp
  src/log/model_logger.cpp
  src/log/pipeline_logger.cpp
  src/log/pipeline_gie_logger.cpp
  src/log/pipeline_analyzer_logger.cpp
  src/log/server_logger.cpp
  src/log/preview_logger.cpp
  src/log/event_logger.cpp
  src/log/notice_logger.cpp)
target_include_directories(backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${RDKAFKA_INCLUDE_DIRS})
target_link_libraries(backend PRIVATE Drogon::Drogon spdlog::spdlog libzip::zip yaml-cpp ${RDKAFKA_LIBRARIES})
