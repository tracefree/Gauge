#include "renderer_vulkan.hpp"

#include <gauge/core/app.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdint>
#include <cassert>
#include <print>

using namespace Gauge;

extern App* gApp;

bool RendererVulkan::initialize(SDL_Window* p_sdl_window) {
    // Instance
    vkb::InstanceBuilder builder;
    builder = builder
        .request_validation_layers()
        .use_default_debug_messenger()
        .set_app_name(gApp->name.c_str())
        .set_app_version(0, 1, 0)
        .set_engine_name("Gauge")
        .set_engine_version(0, 1, 0)
        .require_api_version(1, 3, 0)
        .set_minimum_instance_version(1, 3, 0);
    
    uint sdl_extension_count {0};
    char const * const * extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    builder.enable_extensions(sdl_extension_count, extensions);

    auto instance_ret = builder.build();
    if (!instance_ret) {
        return false;
    }
    instance = instance_ret.value();
    create_surface(p_sdl_window);

    // Features
    VkPhysicalDeviceVulkan13Features device_features_13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .robustImageAccess = VK_FALSE,
        .inlineUniformBlock = VK_FALSE,
        .descriptorBindingInlineUniformBlockUpdateAfterBind = VK_FALSE,
        .pipelineCreationCacheControl = VK_FALSE,
        .privateData = VK_FALSE,
        .shaderDemoteToHelperInvocation = VK_FALSE,
        .shaderTerminateInvocation = VK_FALSE,
        .subgroupSizeControl = VK_FALSE,
        .computeFullSubgroups = VK_FALSE,
        .synchronization2 = VK_TRUE,
        .textureCompressionASTC_HDR = VK_FALSE,
        .shaderZeroInitializeWorkgroupMemory = VK_FALSE,
        .dynamicRendering = VK_TRUE,
        .shaderIntegerDotProduct = VK_FALSE,
        .maintenance4 = VK_TRUE,
    };

    // Physical device
    vkb::PhysicalDeviceSelector selector{ instance };
    auto physical_device_ret = selector
                        .set_surface(surface)
                        .set_minimum_version(1, 3)
                        .set_required_features_13(device_features_13)
                        .add_required_extension("VK_EXT_shader_object")
                        .select();
    if (!physical_device_ret) {
        return false;
    }
    physical_device = physical_device_ret.value();

    // Device
    vkb::DeviceBuilder device_builder{ physical_device };
    auto device_ret = device_builder.build();
    if (!device_ret) {
        return false;
    }
    device = device_ret.value ();

    if (volkInitialize() != VK_SUCCESS) {
        return false;
    }

    volkLoadInstance(instance.instance);
    volkLoadDevice(device.device);

    // Graphics Queue
    auto graphics_queue_ret = device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret)  {
        return false;
    }
    graphics_queue = graphics_queue_ret.value ();

    // Frames in flight
    frames_in_flight.reserve(max_frames_in_flight);
    for (uint i = 0; i < max_frames_in_flight; i++) {
        VkCommandPool cmd_pool = create_command_pool();
        VkCommandBuffer cmd = create_command_buffer(cmd_pool);
        frames_in_flight.emplace_back(FrameData {
            .cmd_pool = cmd_pool,
            .cmd = cmd,
        });
    }

    // Swapchain
    vkb::SwapchainBuilder swapchain_builder {device};
    auto swapchain_ret = swapchain_builder
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(1920, 1080)
        .build();
    if (!swapchain_ret) {
        return false;
    }
    vkb::Swapchain vkb_swapchain = swapchain_ret.value();
    swapchain = vkb_swapchain.swapchain;
    swapchain_images = vkb_swapchain.get_images().value();
    swapchain_image_views = vkb_swapchain.get_image_views().value();

    initialized = true;
    return true;
}

VkCommandPool RendererVulkan::create_command_pool() const {
    const VkCommandPoolCreateInfo cmd_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VkCommandPool cmd_pool;
    VkResult result = vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &cmd_pool);
    return cmd_pool;
}

VkCommandBuffer RendererVulkan::create_command_buffer(VkCommandPool p_cmd_pool) const {
    const VkCommandBufferAllocateInfo cmd_allocate_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = p_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd;
    VkResult result = vkAllocateCommandBuffers(device, &cmd_allocate_info, &cmd);
    return cmd;
}

void RendererVulkan::draw() {
    uint next_image_index = 0;
    vkAcquireNextImageKHR(device.device, swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &next_image_index);

    VkCommandBuffer cmd = get_current_frame().cmd;
    vkResetCommandBuffer(cmd, 0);

    const VkCommandBufferBeginInfo cmd_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr,
    };
    VkResult result = vkBeginCommandBuffer(cmd, &cmd_begin_info);
    // -- Begin recording commands ---

    VkRenderingAttachmentInfo rendering_attachement_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = swapchain_image_views[0],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_NONE,
        .clearValue = {
            .color = {1.0f, 0.0f, 0.0f, 1.0f},
        }
    };

    VkRenderingInfo rendering_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width = 1920,
                .height = 1080,
            },
        },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &rendering_attachement_info,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };
    
    vkCmdBeginRendering(cmd, &rendering_info);

    vkCmdEndRendering(cmd);

    // --- End recording commands ---
    vkEndCommandBuffer(cmd);

    const VkCommandBufferSubmitInfo cmd_buffer_submit_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };

    const VkSubmitInfo2 submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = 0,
        .pWaitSemaphoreInfos = nullptr,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_buffer_submit_info,
        .signalSemaphoreInfoCount = 0,
        .pSignalSemaphoreInfos = nullptr,
    };
    vkQueueSubmit2(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);

    VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &next_image_index,
        .pResults = nullptr,
    };

    vkQueuePresentKHR(graphics_queue, &present_info);

    current_frame_index++;
}

vkb::Instance const* RendererVulkan::get_instance() const {
    return &instance;
}

RendererVulkan::FrameData RendererVulkan::get_current_frame() const {
    return frames_in_flight[current_frame_index % max_frames_in_flight];
}

void RendererVulkan::create_surface(SDL_Window* p_window) {
    bool success = SDL_Vulkan_CreateSurface(p_window, instance.instance, nullptr, &surface);
    if (!success) {
        std::println("{}", SDL_GetError());
    }
}