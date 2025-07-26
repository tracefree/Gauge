#include "renderer_vulkan.hpp"

#include "VkBootstrap.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <print>
#include <string>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <gauge/core/app.hpp>

#define CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK 1

using namespace Gauge;

extern App* gApp;

#ifdef CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK
VkBool32 validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT p_message_severity,
    VkDebugUtilsMessageTypeFlagsEXT p_message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data) {
    const char* severity = vkb::to_string_message_severity(p_message_severity);
    const char* type = vkb::to_string_message_type(p_message_type);
    std::println("[{}] {}: {}", type, severity, p_callback_data->pMessage);
    return VK_FALSE;
}
#endif

std::expected<void, std::string>
RendererVulkan::initialize(SDL_Window* p_sdl_window) {
    // Instance
    vkb::InstanceBuilder instance_builder;
    instance_builder = instance_builder
                           .request_validation_layers()
#ifdef CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK
                           .set_debug_callback(validation_layer_callback)
#else
                           .use_default_debug_messenger()
#endif  // CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK
                           .set_app_name(gApp->name.c_str())
                           .set_app_version(0, 1, 0)
                           .set_engine_name("Gauge")
                           .set_engine_version(0, 1, 0)
                           .require_api_version(1, 3, 0)
                           .set_minimum_instance_version(1, 3, 0);

    uint sdl_extension_count{0};
    char const* const* extensions =
        SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    instance_builder.enable_extensions(sdl_extension_count, extensions);

    auto instance_ret = instance_builder.build();
    if (!instance_ret) {
        return std::unexpected("Could not create Vulkan instance.");
    }
    instance = instance_ret.value();

    // Surface
    auto surface_res = create_surface(p_sdl_window);
    if (!surface_res) {
        return surface_res;
    }

    // Features
    VkPhysicalDeviceVulkan13Features device_features_13{
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
    vkb::PhysicalDeviceSelector selector{instance};
    auto physical_device_ret =
        selector.set_surface(surface)
            .set_minimum_version(1, 3)
            .set_required_features_13(device_features_13)
            .add_required_extension("VK_EXT_shader_object")
            .select();
    if (!physical_device_ret) {
        return std::unexpected("Could not create Vulkan physical device.");
    }
    physical_device = physical_device_ret.value();

    // Device
    vkb::DeviceBuilder device_builder{physical_device};
    auto device_ret = device_builder.build();
    if (!device_ret) {
        return std::unexpected("Could not create Vulkan logical device.");
    }
    device = device_ret.value();

    if (volkInitialize() != VK_SUCCESS) {
        return std::unexpected("Could not initialize volk.");
    }

    volkLoadInstance(instance.instance);
    volkLoadDevice(device.device);

    // Graphics Queue
    auto graphics_queue_ret = device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        return std::unexpected("Could not get graphics queue.");
    }
    graphics_queue = graphics_queue_ret.value();

    // Frames in flight
    frames_in_flight.reserve(max_frames_in_flight);
    for (uint i = 0; i < max_frames_in_flight; i++) {
        FrameData frame{};
        auto result = create_command_pool()
                          .and_then([this, &frame](VkCommandPool p_cmd_pool) {
                              frame.cmd_pool = p_cmd_pool;
                              return create_command_buffer(frame.cmd_pool);
                          })
                          .transform([&frame](VkCommandBuffer p_cmd) {
                              frame.cmd = p_cmd;
                          });
        if (!result) {
            return result;
        }

        VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        VK_CHECK_RET(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.present_complete_semaphore),
                     "Could not create presentation complete semaphore.");
        VK_CHECK_RET(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.render_complete_semaphore),
                     "Could not create render complete semaphore.");

        VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VK_CHECK_RET(vkCreateFence(device, &fence_info, nullptr, &frame.draw_fence), "Could not create render fence.");
    }

    // Swapchain
    auto swapchain_res = create_swapchain(p_sdl_window);
    if (!swapchain_res) {
        return swapchain_res;
    }

    initialized = true;
    return {};
}

std::expected<void, std::string>
RendererVulkan::create_swapchain(SDL_Window* p_sdl_window, VkSwapchainKHR old_swapchain) {
    int window_width, window_height = 0;
    SDL_GetWindowSize(p_sdl_window, &window_width, &window_height);

    vkb::SwapchainBuilder swapchain_builder{device};
    if (old_swapchain != VK_NULL_HANDLE) {
        swapchain_builder.set_old_swapchain(old_swapchain);
    }
    auto swapchain_ret =
        swapchain_builder
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(window_width, window_height)
            .set_desired_min_image_count(3)
            .build();
    if (!swapchain_ret) {
        return std::unexpected("Could not create swapchain.");
    }
    vkb::Swapchain vkb_swapchain = swapchain_ret.value();
    swapchain_data.swapchain = vkb_swapchain.swapchain;
    swapchain_data.images = vkb_swapchain.get_images().value();
    swapchain_data.image_views = vkb_swapchain.get_image_views().value();
    swapchain_data.extent = vkb_swapchain.extent;
    swapchain_data.image_format = vkb_swapchain.image_format;
    return {};
}

std::expected<VkCommandPool, std::string>
RendererVulkan::create_command_pool() const {
    const VkCommandPoolCreateInfo cmd_pool_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VkCommandPool cmd_pool;
    VK_CHECK_RET(
        vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &cmd_pool),
        "Could not create command pool");

    return cmd_pool;
}

std::expected<VkCommandBuffer, std::string>
RendererVulkan::create_command_buffer(
    VkCommandPool p_cmd_pool) const {
    const VkCommandBufferAllocateInfo cmd_allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = p_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd;
    VK_CHECK_RET(vkAllocateCommandBuffers(device, &cmd_allocate_info, &cmd),
                 "Could not allocate command buffer");

    return cmd;
}

void RendererVulkan::draw() {
    uint next_image_index = 0;
    VkResult acquisition_res = vkAcquireNextImageKHR(device.device, swapchain_data.swapchain, UINT64_MAX, VK_NULL_HANDLE,
                                                     VK_NULL_HANDLE, &next_image_index);

    VkCommandBuffer cmd = get_current_frame().cmd;
    VK_CHECK(vkResetCommandBuffer(cmd, 0),
             "Could not reset command buffer: Out of device memory?");

    const VkCommandBufferBeginInfo cmd_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info),
             "Could not begin command buffer");
    // -- Begin recording commands ---

    VkRenderingAttachmentInfo rendering_attachement_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = swapchain_data.image_views[0],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_NONE,
        .clearValue = {
            .color = {{1.0f, 0.5f, 0.0f, 1.0f}},
        }};

    VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea =
            {
                .offset =
                    {
                        .x = 0,
                        .y = 0,
                    },
                .extent =
                    {
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

    const VkCommandBufferSubmitInfo cmd_buffer_submit_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };

    const VkSubmitInfo2 submit_info{
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
    VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit_info, VK_NULL_HANDLE),
             "Could not submit command buffer to graphics queue");

    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_data.swapchain,
        .pImageIndices = &next_image_index,
        .pResults = nullptr,
    };

    VkResult present_result = vkQueuePresentKHR(graphics_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR) [[unlikely]] {
        // recreate swapchain
    }

    current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
}

vkb::Instance const*
RendererVulkan::get_instance() const {
    return &instance;
}

RendererVulkan::FrameData
RendererVulkan::get_current_frame() const {
    return frames_in_flight[current_frame_index % max_frames_in_flight];
}

std::expected<void, std::string>
RendererVulkan::create_surface(SDL_Window* p_window) {
    if (!SDL_Vulkan_CreateSurface(p_window, instance.instance, nullptr,
                                  &surface)) [[unlikely]] {
        return std::unexpected(SDL_GetError());
    }
    return {};
}
