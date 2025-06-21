cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(simple_web_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Os -Wall -Wextra -Wpedantic -Werror -DDEBUG" CACHE STRING "Debug flags" FORCE)
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -UDEBUG" CACHE STRING "Release flags" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

set(CMAKE_CXX_CLANG_TIDY
    clang-tidy
    --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
    --format-style=file
)

add_executable(simple_web_server 
    src/thread_pool.cpp    
    src/main.cpp
    src/server.cpp
)

target_include_directories(simple_web_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
