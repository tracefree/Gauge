#include "renderer_vulkan.hpp"

#include "VkBootstrap.h"

#include <expected>
#include <gauge/common.hpp>
#include <gauge/core/app.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <cassert>
#include <cstdint>
#include <format>
#include <print>
#include <string>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>
#include <Tracy/tracy/Tracy.hpp>
#include <Tracy/tracy/TracyVulkan.hpp>

#define TRACY_VK_USE_SYMBOL_TABLE
#define CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK 1
#define USE_VULKAN_DEBUG 1

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
    std::println("[{}] {}: {}\n", type, severity, p_callback_data->pMessage);
    return VK_FALSE;
}
#endif

static std::expected<VkSurfaceKHR, std::string>
CreateSurface(vkb::Instance p_instance, SDL_Window* p_window) {
    VkSurfaceKHR r_surface;
    if (!SDL_Vulkan_CreateSurface(p_window, p_instance.instance, nullptr,
                                  &r_surface)) [[unlikely]] {
        return std::unexpected(SDL_GetError());
    }
    return r_surface;
}

static std::expected<vkb::Instance, std::string>
CreateInstance() {
    vkb::InstanceBuilder instance_builder;
    instance_builder = instance_builder
#ifdef USE_VULKAN_DEBUG
                           .request_validation_layers()
#ifdef CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK
                           .set_debug_callback(validation_layer_callback)
#else
                           .use_default_debug_messenger()
#endif  // CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK
#endif  // USE_VULKAN_DEBUG
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
        return std::unexpected(std::format("Could not create Vulkan instance. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                           instance_ret.full_error().type.value(),
                                           instance_ret.full_error().type.message(),
                                           string_VkResult(instance_ret.full_error().vk_result)));
    }
    return instance_ret.value();
}

static std::expected<vkb::PhysicalDevice, std::string>
CreatePhysicalDevice(vkb::Instance p_instance, VkSurfaceKHR p_surface) {
    // Features
    VkPhysicalDeviceVulkan12Features device_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = nullptr,
        .timelineSemaphore = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features device_features_13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };

    // Physical device
    vkb::PhysicalDeviceSelector selector{p_instance};
    auto physical_device_ret =
        selector.set_surface(p_surface)
            .set_minimum_version(1, 3)
            .set_required_features_12(device_features_12)
            .set_required_features_13(device_features_13)
            .add_required_extension("VK_EXT_shader_object")
            .select();
    if (!physical_device_ret) {
        return std::unexpected(std::format("Could not create Vulkan physical device. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                           physical_device_ret.full_error().type.value(),
                                           physical_device_ret.full_error().type.message(),
                                           string_VkResult(physical_device_ret.full_error().vk_result)));
    }
    return physical_device_ret.value();
}

static std::expected<vkb::Device, std::string>
CreateDevice(vkb::PhysicalDevice p_physical_device) {
    vkb::DeviceBuilder device_builder{p_physical_device};
    auto device_ret = device_builder.build();
    if (!device_ret) {
        return std::unexpected(std::format("Could not create Vulkan logical device. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                           device_ret.full_error().type.value(),
                                           device_ret.full_error().type.message(),
                                           string_VkResult(device_ret.full_error().vk_result)));
    }
    return device_ret.value();
}

static std::expected<void, std::string>
InitializeVolk(vkb::Instance p_instance, vkb::Device p_device) {
    VK_CHECK_RET(volkInitialize(), "Could not initialize volk");
    volkLoadInstance(p_instance.instance);
    volkLoadDevice(p_device.device);
    return {};
}

static std::expected<VkQueue, std::string>
GetQueue(vkb::Device p_device) {
    auto graphics_queue_ret = p_device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        return std::unexpected(std::format("Could not get graphics queue. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                           graphics_queue_ret.full_error().type.value(),
                                           graphics_queue_ret.full_error().type.message(),
                                           string_VkResult(graphics_queue_ret.full_error().vk_result)));
    }
    return graphics_queue_ret.value();
}

std::expected<VkCommandPool, std::string>
RendererVulkan::CreateCommandPool() const {
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
RendererVulkan::CreateCommandBuffer(
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

std::expected<void, std::string>
RendererVulkan::CreateFrameData() {
    frames_in_flight.reserve(max_frames_in_flight);
    for (uint i = 0; i < max_frames_in_flight; i++) {
        FrameData frame{};
        CHECK_RET(CreateCommandPool()
                      .and_then([this, &frame](VkCommandPool p_cmd_pool) {
                          frame.cmd_pool = p_cmd_pool;
                          return CreateCommandBuffer(frame.cmd_pool);
                      })
                      .transform([&frame](VkCommandBuffer p_cmd) {
                          frame.cmd = p_cmd;
                      }));

        VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VK_CHECK_RET(vkCreateFence(device, &fence_info, nullptr, &frame.queue_submit_fence),
                     "Could not create render fence");

        VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
        };
        VK_CHECK_RET(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.swapchain_acquire_semaphore),
                     "Could not create acquire semaphore");

        frames_in_flight.emplace_back(frame);

        SetDebugName((uint64_t)frame.cmd_pool, VK_OBJECT_TYPE_COMMAND_POOL, std::format("Primary command pool [{}]", i));
        SetDebugName((uint64_t)frame.cmd, VK_OBJECT_TYPE_COMMAND_BUFFER, std::format("Primary command buffer [{}]", i));
        SetDebugName((uint64_t)frame.queue_submit_fence, VK_OBJECT_TYPE_FENCE, std::format("Queue submit fence [{}]", i));
        SetDebugName((uint64_t)frame.swapchain_acquire_semaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain acquire semaphore [{}]", i));
    }
    return {};
}

std::expected<void, std::string>
RendererVulkan::CreateSwapchain(bool recreate) {
    int window_width, window_height = 0;
    SDL_GetWindowSize(window, &window_width, &window_height);

    vkb::SwapchainBuilder swapchain_builder{device};
    if (recreate) {
        swapchain_builder.set_old_swapchain(swapchain.vkb_swapchain);
        vkDeviceWaitIdle(device);
    }
    auto swapchain_ret =
        swapchain_builder
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(window_width, window_height)
            .set_desired_min_image_count(3)
            .build();
    if (!swapchain_ret) {
        swapchain.handle = VK_NULL_HANDLE;
        return std::unexpected(std::format("Could not create swapchain. vk-bootstrap error code: [{}] {}. Vulkan result: {}",
                                           swapchain_ret.full_error().type.value(),
                                           swapchain_ret.full_error().type.message(),
                                           string_VkResult(swapchain_ret.full_error().vk_result)));
    }

    if (recreate) {
        vkb::destroy_swapchain(swapchain.vkb_swapchain);
    }

    swapchain.vkb_swapchain = swapchain_ret.value();
    swapchain.handle = swapchain.vkb_swapchain.swapchain;
    swapchain.images = swapchain.vkb_swapchain.get_images().value();
    swapchain.image_views = swapchain.vkb_swapchain.get_image_views().value();
    swapchain.extent = swapchain.vkb_swapchain.extent;
    swapchain.image_format = swapchain.vkb_swapchain.image_format;

    for (VkSemaphore old_semaphore : swapchain_release_semaphores) {
        vkDestroySemaphore(device, old_semaphore, nullptr);
    }
    swapchain_release_semaphores.resize(swapchain.vkb_swapchain.image_count);

    VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
    };
    for (uint i = 0; i < swapchain.vkb_swapchain.image_count; ++i) {
        VK_CHECK_RET(vkCreateSemaphore(device, &semaphore_info, nullptr, &swapchain_release_semaphores[i]),
                     "Could not create acquire semaphore");

        SetDebugName((uint64_t)swapchain_release_semaphores[i], VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain release semaphore [{}]", i));
    }

    SetDebugName((uint64_t)swapchain.handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Swapchain");
#ifdef USE_VULKAN_DEBUG
    for (uint i = 0; i < swapchain.vkb_swapchain.image_count; ++i) {
        SetDebugName((uint64_t)swapchain.images[i], VK_OBJECT_TYPE_IMAGE, std::format("Swapchain image [{}]", i));
        SetDebugName((uint64_t)swapchain.image_views[i], VK_OBJECT_TYPE_IMAGE_VIEW, std::format("Swapchain image view [{}]", i));
    }
#endif  // USE_VULKAN_DEBUG

    return {};
}

std::expected<void, std::string>
RendererVulkan::Initialize(SDL_Window* p_sdl_window) {
    window = p_sdl_window;

    CHECK_RET(
        CreateInstance()
            .and_then([this, p_sdl_window](vkb::Instance p_instance) {
                instance = p_instance;
                return CreateSurface(p_instance, p_sdl_window);
            })
            .and_then([this](VkSurfaceKHR p_surface) {
                surface = p_surface;
                return CreatePhysicalDevice(instance, surface);
            })
            .and_then([this](vkb::PhysicalDevice p_physical_device) {
                physical_device = p_physical_device;
                return CreateDevice(p_physical_device);
            })
            .and_then([this](vkb::Device p_device) {
                device = p_device;
                return InitializeVolk(instance, device);
            })
            .and_then([this]() {
                SetDebugName((uint64_t)instance.instance, VK_OBJECT_TYPE_INSTANCE, "Primary instance");
                SetDebugName((uint64_t)physical_device.physical_device, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "Primary physical device");
                SetDebugName((uint64_t)device.device, VK_OBJECT_TYPE_DEVICE, "Primary device");
                SetDebugName((uint64_t)surface, VK_OBJECT_TYPE_SURFACE_KHR, "Main window surface");

                return GetQueue(device);
            })
            .and_then([this](VkQueue p_queue) {
                graphics_queue = p_queue;
                SetDebugName((uint64_t)graphics_queue, VK_OBJECT_TYPE_QUEUE, "Graphics queue");
                return CreateFrameData();
            })
            .and_then([this]() {
                return CreateSwapchain();
            }));

    initialized = true;
    return {};
}

void RendererVulkan::Draw() {
    ZoneScoped;

    FrameData current_frame = GetCurrentFrame();
    uint next_image_index = 0;

    while (vkWaitForFences(device, 1, &current_frame.queue_submit_fence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT)
        ;

    // TODO: Check result value and recreate swapchain if necessary
    vkAcquireNextImageKHR(device.device, swapchain.handle, UINT64_MAX, current_frame.swapchain_acquire_semaphore,
                          VK_NULL_HANDLE, &next_image_index);

    VK_CHECK(vkResetFences(device, 1, &current_frame.queue_submit_fence),
             "Could not reset queue submit fence");
    VK_CHECK(vkResetCommandPool(device, current_frame.cmd_pool, 0),
             "Could not reset command pool");

    VkCommandBuffer current_command_buffer = current_frame.cmd;
    CommandBufferVulkan cmd{current_command_buffer};

    // tracy::VkCtx* tracy_context = TracyVkContext(physical_device, device, graphics_queue, cmd);

    CHECK(cmd.Begin());

    // -- Begin recording commands ---
    {
        // TracyVkZone(tracy_context, cmd, "Clear window");
        cmd.transition_image(swapchain.images[next_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo rendering_attachement_info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = swapchain.image_views[next_image_index],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .color = {{0.0f, 0.0f, 0.0f, 0.0f}},
            }};

        int window_width, window_height = 0;
        SDL_GetWindowSize(window, &window_width, &window_height);
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
                            .width = (uint)window_width,  // TODO
                            .height = (uint)window_height,
                        },
                },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &rendering_attachement_info,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr,
        };

        vkCmdBeginRendering(current_command_buffer, &rendering_info);

        vkCmdEndRendering(current_command_buffer);
        cmd.transition_image(swapchain.images[next_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    // TracyVkCollect(tracy_context, cmd);

    // --- End recording commands ---
    CHECK(cmd.End());

    // Submit to graphics queue
    const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &current_frame.swapchain_acquire_semaphore,
        .pWaitDstStageMask = &wait_dst_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &current_command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &swapchain_release_semaphores[next_image_index],
    };
    VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, current_frame.queue_submit_fence),
             "Could not submit command buffer to graphics queue");

    // Present
    const VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain_release_semaphores[next_image_index],
        .swapchainCount = 1,
        .pSwapchains = &swapchain.handle,
        .pImageIndices = &next_image_index,
        .pResults = nullptr,
    };

    const VkResult present_result = vkQueuePresentKHR(graphics_queue, &present_info);
    FrameMark;
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        CHECK(CreateSwapchain(true));
    }

    current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
}

vkb::Instance const*
RendererVulkan::GetInstance() const {
    return &instance;
}

RendererVulkan::FrameData
RendererVulkan::GetCurrentFrame() const {
    return frames_in_flight[current_frame_index];
}

void RendererVulkan::SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const {
#ifdef USE_VULKAN_DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = p_type,
        .objectHandle = p_handle,
        .pObjectName = p_name.c_str(),
    };
    vkSetDebugUtilsObjectNameEXT(device, &name_info);
#endif  // USE_VULKAN_DEBUG
}

void RendererVulkan::OnWindowResized() {
    CHECK(CreateSwapchain(true));
}