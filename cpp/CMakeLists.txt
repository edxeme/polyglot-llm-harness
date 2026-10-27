cmake_minimum_required(VERSION 3.20)
project(lmh CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(lmh
    src/main.cpp
    src/json.cpp
    src/report.cpp
    src/registry.cpp
    src/runner.cpp
)
target_include_directories(lmh PRIVATE src)

if(APPLE OR UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(lmh PRIVATE Threads::Threads)
endif()
