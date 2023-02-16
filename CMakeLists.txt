cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(Muduo_WebServer LANGUAGES CXX)

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(WARNING "The binary directory of CMake cannot be the same as source directory!")
endif()

if(WIN32)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

if(NOT MSVC)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found CCache: ${CCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
    endif()
endif()

#set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(http)
add_subdirectory(mymuduo)