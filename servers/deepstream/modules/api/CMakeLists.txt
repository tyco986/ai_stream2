cmake_minimum_required(VERSION 3.16)

project(deepstream_api)

find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
find_package(Threads REQUIRED)

add_executable(deepstream_api
    main.cpp
    constants.cpp
    json.cpp
    pipeline_service.cpp
    api_server.cpp
)
target_compile_features(deepstream_api PRIVATE cxx_std_17)
target_include_directories(deepstream_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${YAML_CPP_INCLUDE_DIRS}
)
target_compile_definitions(deepstream_api PRIVATE CPPHTTPLIB_NO_OPENSSL)
target_compile_options(deepstream_api PRIVATE -Wall -Werror ${YAML_CPP_CFLAGS_OTHER})
target_link_directories(deepstream_api PRIVATE ${YAML_CPP_LIBRARY_DIRS})
target_link_libraries(deepstream_api PRIVATE
    ${YAML_CPP_LIBRARIES}
    Threads::Threads
)
