cmake_minimum_required(VERSION 3.0)
project(Rowan)

set (LIB_NAME "rowan")
set (CMAKE_C_COMPILER "g++")
set (CMAKE_CXX_COMPILER ${CMAKE_C_COMPILER})

set(CMAKE_C_FLAGS "-Wall -Wno-deprecated-declarations -Wsign-compare -march=native -O3 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 ")

option (SANITIZE "Turn on sanitization" OFF)
if (SANITIZE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

set(INCLUDE_BASE ${PROJECT_SOURCE_DIR}/include)
include_directories(${INCLUDE_BASE})

#Source file define
set(COMMON_SRC ${PROJECT_SOURCE_DIR}/src)

#Used by both server and clients
file(GLOB_RECURSE COMMON_FILE ${COMMON_SRC}/*.cpp)
add_library(${LIB_NAME} STATIC ${COMMON_FILE})
target_link_libraries(${LIB_NAME} PUBLIC pthread ibverbs memcached profiler)
link_libraries(${LIB_NAME})
target_include_directories(${LIB_NAME} PUBLIC ${PROJECT_BINARY_DIR})

file(GLOB TEST_SRC ${PROJECT_SOURCE_DIR}/test/*.cpp)
foreach (TEST ${TEST_SRC})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    add_executable(${TEST_NAME} ${TEST})
    target_link_libraries(${TEST_NAME} ${LINKS_FLAGS})
endforeach()

