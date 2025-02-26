#pragma once

#include <VkBootstrap.h>

namespace Gauge {
    struct Renderer {
        static vkb::Instance instance;
        static vkb::PhysicalDevice physical_device;
        static vkb::Device device;
        static VkQueue graphics_queue;

        static bool initialize();
    };
}