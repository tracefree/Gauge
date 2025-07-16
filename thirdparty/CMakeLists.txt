# VulkanBootstrap
add_subdirectory(vk-bootstrap)

# volk
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
add_subdirectory(volk)

# yaml-cpp
set(YAML_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
set(YAML_CPP_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_subdirectory(yaml-cpp)