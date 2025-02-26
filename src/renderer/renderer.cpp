#include "renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <print>

using namespace Gauge;

vkb::Instance Renderer::instance;
vkb::PhysicalDevice Renderer::physical_device;
vkb::Device Renderer::device;
VkQueue Renderer::graphics_queue;

bool Renderer::initialize() {
    // Instance
    vkb::InstanceBuilder builder;
    builder = builder.request_validation_layers().use_default_debug_messenger();
    uint extension_count {0};
    char const * const * extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
    builder.enable_extensions(extension_count, extensions);
    auto instance_ret = builder.build();
    if (!instance_ret) {
        return false;
    }
    Renderer::instance = instance_ret.value();

    // Physical device
    vkb::PhysicalDeviceSelector selector{ Renderer::instance };
    auto physical_device_ret = selector.defer_surface_initialization()
                        .set_minimum_version(1, 2)
                        .require_dedicated_transfer_queue()
                        .select ();
    if (!physical_device_ret) {
        return false;
    }
    Renderer::physical_device = physical_device_ret.value();

    // Device
    vkb::DeviceBuilder device_builder{ Renderer::physical_device };
    auto device_ret = device_builder.build ();
    if (!device_ret) {
        return false;
    }
    Renderer::device = device_ret.value ();

    // Graphics Queue
    auto graphics_queue_ret = device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret)  {
        return false;
    }
    Renderer::graphics_queue = graphics_queue_ret.value ();

    return true;
}