cmake_minimum_required(VERSION 3.31)
project(gauge VERSION 0.0.1 LANGUAGES CXX)

# Setup
find_program(CCACHE_FOUND ccache)
include(FetchContent)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

# Dependencies

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

# Gauge

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(gauge
    src/core/app.cpp
    src/core/config.cpp
    src/ui/window.cpp
)

target_include_directories(gauge PUBLIC include)

target_link_libraries(gauge PRIVATE
  yaml-cpp::yaml-cpp
  SDL3::SDL3
)