cmake_minimum_required(VERSION 3.31)
project(sandbox VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
include(FetchContent)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS True)

# --- Dependencies ---
# Gauge
add_subdirectory(gauge)

# --- Project ---
add_compile_options("-g")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(sandbox
    src/main.cpp
    src/game.cpp
)

target_link_libraries(sandbox gauge)
