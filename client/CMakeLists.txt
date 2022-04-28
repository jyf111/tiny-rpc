cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(tinyrpc LANGUAGES C CXX)

add_executable(client)

target_sources(client
PUBLIC
  client.cpp
)

target_compile_options(client
PUBLIC
  -Wall
  -Werror
  -Wunreachable-code
)

target_link_libraries(client
PUBLIC
  -pthread
)

target_compile_definitions(client
PUBLIC
  ASIO_STANDALONE
)

target_include_directories(client
PRIVATE
  ../include/asio/include/
  ../include/
)