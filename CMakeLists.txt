cmake_minimum_required(VERSION 3.31)
project(gauge VERSION 0.0.1 LANGUAGES CXX)

# --- Setup ---
find_program(CCACHE_FOUND ccache)
include(FetchContent)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

# --- Dependencies ---

# SDL3
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)

# Vulkan
find_package(Vulkan REQUIRED)

# VulkanBootstrap
FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        v1.4.307
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)

# yaml-cpp
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

# --- Gauge ---
add_compile_options("-g")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(gauge
    gauge/core/app.cpp
    gauge/core/config.cpp
    gauge/ui/window.cpp
    gauge/renderer/renderer.cpp
)

target_include_directories(gauge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gauge PUBLIC
  SDL3::SDL3
  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  yaml-cpp::yaml-cpp
)