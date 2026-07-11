cmake_minimum_required(VERSION 3.16)
project(dns-server LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" ON)

if(ENABLE_SANITIZERS)
  set(SANITIZER_FLAGS "-fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

set(SOURCES src/main.cpp src/mapper.cpp src/parser.cpp src/response_builder.cpp
            src/server.cpp)

set(HEADERS src/common.hpp src/macros.hpp src/mapper.hpp src/parser.hpp
            src/response_builder.hpp src/server.hpp)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Minimal socket
set(BUILD_MinimalCppSocket_SAMPLES
    OFF
    CACHE BOOL "" FORCE)

FetchContent_Declare(
  min_sock
  GIT_REPOSITORY https://github.com/andreacasalino/Minimal-Socket
  GIT_TAG master)

FetchContent_MakeAvailable(min_sock)
target_link_libraries(${PROJECT_NAME} PRIVATE MinimalSocket)

# Thread pool
FetchContent_Declare(
  bshoshany_thread_pool
  GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
  GIT_TAG v5.0.0
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE EXCLUDE_FROM_ALL SYSTEM)

FetchContent_MakeAvailable(bshoshany_thread_pool)
add_library(BS_thread_pool INTERFACE)
target_include_directories(
  BS_thread_pool INTERFACE ${bshoshany_thread_pool_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE BS_thread_pool)

# Arg parser

FetchContent_Declare(argparse
                     GIT_REPOSITORY https://github.com/p-ranav/argparse.git)
FetchContent_MakeAvailable(argparse)
target_link_libraries(${PROJECT_NAME} PRIVATE argparse)
