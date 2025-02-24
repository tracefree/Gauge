cmake_minimum_required(VERSION 3.31)
project(gauge VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(gauge
    src/gauge.cpp
)

target_include_directories(gauge PUBLIC include)