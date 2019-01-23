#/*
# * Keith Skinner
# * Lab: Project
# * Date: 12/13/2018
# */

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(simfs VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -g -O0 -D_DEBUG")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(simfs src/simfs.c src/simfs.test.c)

find_package(FUSE)
if(${FUSE_FOUND})
    include_directories(${FUSE_INCLUDE_DIR})
    target_link_libraries(simfs ${FUSE_LIBRARIES})
else()
    target_include_directories(simfs PRIVATE fuse)
endif()
