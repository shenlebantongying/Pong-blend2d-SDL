cmake_minimum_required(VERSION 3.27)
project(blend2d_pong)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

set(BLEND2D_STATIC TRUE)
include("${CMAKE_SOURCE_DIR}/blend2d/CMakeLists.txt")

configure_file(${CMAKE_SOURCE_DIR}/IntelOneMono.ttf ${CMAKE_BINARY_DIR}/IntelOneMono.ttf COPYONLY)

add_executable(blend2d_pong src/main.cpp)

set_target_properties(blend2d_pong PROPERTIES COMPILE_WARNING_AS_ERROR ON)

target_link_libraries(blend2d_pong blend2d::blend2d SDL2::SDL2)

target_compile_options(blend2d_pong PRIVATE -Wall -Wextra  -Werror)
