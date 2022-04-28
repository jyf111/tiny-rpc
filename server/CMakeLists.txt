cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(tinyrpc LANGUAGES C CXX)

add_executable(server)

target_sources(server
PUBLIC
  server.cpp
)

target_compile_options(server
PUBLIC
  -Wall
  -Werror
  -Wunreachable-code
)

target_link_libraries(server
PUBLIC
  -pthread
)

target_compile_definitions(server
PUBLIC
  ASIO_STANDALONE
)

target_include_directories(server
PRIVATE
  ../include/asio/include/
  ../include/
)