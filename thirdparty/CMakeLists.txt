# Disable warnings originating from third party dependencies
add_compile_options(-w)

# VulkanBootstrap
add_subdirectory(vk-bootstrap)

# volk
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
add_subdirectory(volk)

# yaml-cpp
set(YAML_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
set(YAML_CPP_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_subdirectory(yaml-cpp)

#tracy
set(TRACY_LTO ON)
option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory(tracy)
