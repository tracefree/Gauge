# Disable warnings originating from third party dependencies
add_compile_options(-w)

# VulkanBootstrap
add_subdirectory(vk-bootstrap)

# volk
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
add_subdirectory(volk)

# yaml-cpp
set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
set(YAML_CPP_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_subdirectory(yaml-cpp)

# Tracy
set(TRACY_LTO ON)
option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(tracy)

# glm
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_subdirectory(glm)

# ImGUI
add_compile_definitions(IMGUI_IMPL_VULKAN_USE_VOLK)
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
    imgui/
    imgui/backends/
)
