cmake_minimum_required(VERSION 3.31)
project(gauge VERSION 0.0.1 LANGUAGES CXX C)

# --- Setup ---
find_program(CCACHE_FOUND ccache)
include(FetchContent)

if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---

# SDL3
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)

# Vulkan
set(BUILD_SHARED_LIBS ON)
find_package(Vulkan REQUIRED)
find_package(VulkanHeaders REQUIRED CONFIG)

# Other
add_subdirectory(thirdparty)

add_compile_options(-Wall)

# --- Gauge ---
add_compile_options("-g")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(gauge_renderer STATIC
  gauge/renderer/renderer.cpp
  gauge/renderer/vulkan/renderer_vulkan.cpp

  thirdparty/volk/volk.c
)

add_library(gauge STATIC
  gauge/core/app.cpp
  gauge/core/config.cpp
  gauge/ui/window.cpp
)

set_source_files_properties(
  thirdparty/volk/volk.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_precompile_headers(gauge_renderer PUBLIC <vector> <print> <string> <unordered_map> <memory>)
target_include_directories(gauge_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(gauge PUBLIC <vector> <print> <string> <unordered_map> <memory>)
target_include_directories(gauge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gauge_renderer PUBLIC
  SDL3::SDL3
  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  volk::volk
)

target_link_libraries(gauge PUBLIC
  gauge_renderer
  SDL3::SDL3
  yaml-cpp::yaml-cpp
)
