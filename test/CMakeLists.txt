cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(test LANGUAGES C CXX)

add_executable(test)

target_sources(test
PUBLIC
  test.cpp
)

target_compile_options(test
PUBLIC
  -g
  -Wall
  -Werror
  -Wunreachable-code
)

target_link_libraries(test
PUBLIC
  -pthread
)

target_compile_definitions(test
PUBLIC
  ASIO_STANDALONE
)

target_include_directories(test
PRIVATE
  ../include/asio/include/
  ../include/
)