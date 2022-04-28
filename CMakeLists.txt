cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(tinyrpc LANGUAGES C CXX)

add_executable(rpc)

target_sources(rpc
PUBLIC
  src/main.cpp
)

target_compile_options(rpc
PUBLIC
  -Wall
  -Werror
  -Wunreachable-code
)

target_link_libraries(rpc
PUBLIC
  -pthread
)

target_compile_definitions(rpc
PUBLIC
  ASIO_STANDALONE
)

target_include_directories(rpc
PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/asio/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
)