cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(simfs VERSION 0.0.1 LANGUAGES C)

add_executable(simfs)
set_property(TARGET simfs PROPERTY C_STANDARD 11)
set_property(TARGET simfs PROPERTY C_STANDARD_REQUIRED ON)
target_compile_options(simfs PRIVATE -Wall -Wextra -Wpedantic)
target_sources(simfs PRIVATE simfs.c simfs.h test_simfs.c)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(FUSE REQUIRED)
include_directories(${FUSE_INCLUDE_DIR})

target_link_libraries(simfs ${FUSE_LIBRARIES})
