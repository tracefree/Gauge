cmake_minimum_required(VERSION 3.31)
project(gauge VERSION 0.0.1 LANGUAGES CXX C)

# --- Setup ---
find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---

include_directories(
  thirdparty/imgui/
  thirdparty/imgui/backends/
)

include_directories(SYSTEM thirdparty/volk)

# SDL3
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
#add_library(SDL3 SHARED IMPORTED)
#set_target_properties(SDL3 PROPERTIES IMPORTED_LOCATION /home/rie/Projects/Gauge/sandbox/lib/libSDL3.so)

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
  gauge/renderer/aabb.cpp
  gauge/renderer/gltf.cpp
  gauge/renderer/renderer.cpp
  gauge/renderer/stb_image_usage.cpp
  gauge/renderer/texture.cpp
  gauge/renderer/shaders/shader.cpp
  gauge/renderer/shaders/debug_line/debug_line_shader.cpp
  gauge/renderer/shaders/pbr/pbr_shader.cpp
  gauge/renderer/shaders/gizmo/gizmo_shader.cpp
  gauge/renderer/vulkan/renderer_vulkan.cpp
  gauge/renderer/vulkan/command_buffer.cpp
  gauge/renderer/vulkan/descriptor.cpp
  gauge/renderer/vulkan/graphics_pipeline_builder.cpp
  gauge/renderer/vulkan/imgui.cpp
  gauge/renderer/vulkan/shader_module.cpp
  gauge/renderer/vulkan/vma_usage.cpp

  thirdparty/volk/volk.c
)

target_compile_definitions(gauge_renderer PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

add_library(gauge STATIC
  gauge/core/app.cpp
  gauge/core/config.cpp
  gauge/core/filesystem.cpp
  gauge/core/string_id.cpp
  gauge/input/input.cpp
  gauge/ui/window.cpp
  gauge/math/transform.cpp
  gauge/math/common.cpp
  gauge/scene/node.cpp
  gauge/scene/scene.cpp
  gauge/scene/scene_tree.cpp
  gauge/components/aabb_gizmo.cpp
  gauge/components/component.cpp
  gauge/components/camera.cpp
  gauge/components/mesh_instance.cpp
  gauge/components/model.cpp
)

set_source_files_properties(
  thirdparty/volk/volk.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_include_directories(gauge_renderer PUBLIC thirdparty/fastgltf/include)
include_directories(gauge_renderer PUBLIC thirdparty/VulkanMemoryAllocator/include)

target_compile_definitions(gauge_renderer PRIVATE VK_NO_PROTOTYPES)
#target_compile_definitions(gauge PRIVATE VK_NO_PROTOTYPES)

target_precompile_headers(gauge_renderer PUBLIC <vector> <print> <string> <unordered_map> <memory>)
target_include_directories(gauge_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(gauge PUBLIC <vector> <print> <string> <unordered_map> <memory>)
target_include_directories(gauge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gauge_renderer PUBLIC
  SDL3
  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  volk::volk
  Tracy::TracyClient
  fastgltf::fastgltf
  GPUOpen::VulkanMemoryAllocator
  glm
  imgui
)

target_link_libraries(gauge PUBLIC
  gauge_renderer
  SDL3
  yaml-cpp::yaml-cpp
  Tracy::TracyClient
  glm
  imgui
)
