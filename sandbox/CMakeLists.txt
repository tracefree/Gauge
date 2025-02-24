cmake_minimum_required(VERSION 3.31)
project(sandbox VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(gauge)

add_executable(sandbox
    src/main.cpp
)

target_link_libraries(sandbox gauge)
