cmake_minimum_required(VERSION 3.31)
project(sandbox VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

add_subdirectory(gauge)

add_executable(sandbox
    src/main.cpp
    src/game.cpp
)

target_link_libraries(sandbox gauge)
