cmake_minimum_required(VERSION 3.31)
project(gauge VERSION 0.0.1 LANGUAGES CXX)

# Dependencies

include(FetchContent)

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
    src/gauge.cpp
    src/config.cpp
)

target_include_directories(gauge PUBLIC include)

target_link_libraries(gauge PRIVATE yaml-cpp::yaml-cpp)