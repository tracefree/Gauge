#include "renderer_vulkan.hpp"

#include "VkBootstrap.h"

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
#include <gauge/core/app.hpp>

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

std::expected<void, std::string>
RendererVulkan::initialize(SDL_Window* p_sdl_window) {
    window = p_sdl_window;

    // Instance
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
    instance = instance_ret.value();

    // Surface
    auto surface_res = create_surface(window);
    if (!surface_res) {
        return surface_res;
    }

    // Features
    VkPhysicalDeviceVulkan12Features device_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = nullptr,
        .timelineSemaphore = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features device_features_13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
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
            .set_required_features_12(device_features_12)
            .set_required_features_13(device_features_13)
            .add_required_extension("VK_EXT_shader_object")
            .select();
    if (!physical_device_ret) {
        ;
        return std::unexpected(std::format("Could not create Vulkan physical device. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                           physical_device_ret.full_error().type.value(),
                                           physical_device_ret.full_error().type.message(),
                                           string_VkResult(physical_device_ret.full_error().vk_result)));
    }
    physical_device = physical_device_ret.value();

    // Device
    vkb::DeviceBuilder device_builder{physical_device};
    auto device_ret = device_builder.build();
    if (!device_ret) {
        return std::unexpected(std::format("Could not create Vulkan logical device. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                           device_ret.full_error().type.value(),
                                           device_ret.full_error().type.message(),
                                           string_VkResult(device_ret.full_error().vk_result)));
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
#ifdef USE_VULKAN_DEBUG
    set_debug_name((uint64_t)graphics_queue, VK_OBJECT_TYPE_QUEUE, "Graphics queue");
#endif

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

        VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        VK_CHECK_RET(vkCreateFence(device, &fence_info, nullptr, &frame.queue_submit_fence),
                     "Could not create render fence");

        VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
        };
        VK_CHECK_RET(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.swapchain_acquire_semaphore),
                     "Could not create acquire semaphore");
        VK_CHECK_RET(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.swapchain_release_semaphore),
                     "Could not create acquire semaphore");

        frames_in_flight.emplace_back(frame);
#ifdef USE_VULKAN_DEBUG
        set_debug_name((uint64_t)frame.cmd_pool, VK_OBJECT_TYPE_COMMAND_POOL, std::format("Primary command pool [{}]", i));
        set_debug_name((uint64_t)frame.cmd, VK_OBJECT_TYPE_COMMAND_BUFFER, std::format("Primary command buffer [{}]", i));
        set_debug_name((uint64_t)frame.queue_submit_fence, VK_OBJECT_TYPE_FENCE, std::format("Queue submit fence [{}]", i));
        set_debug_name((uint64_t)frame.swapchain_acquire_semaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain acquire semaphore [{}]", i));
        set_debug_name((uint64_t)frame.swapchain_release_semaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain release semaphore [{}]", i));
#endif
    }

    // Swapchain
    auto swapchain_res = create_swapchain();
    if (!swapchain_res) {
        return swapchain_res;
    }

    initialized = true;
    return {};
}

std::expected<void, std::string>
RendererVulkan::create_swapchain(bool recreate) {
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
        return std::unexpected("Could not create swapchain.");
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

#ifdef USE_VULKAN_DEBUG
    set_debug_name((uint64_t)swapchain.handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Swapchain");
    for (uint i = 0; i < swapchain.vkb_swapchain.image_count; ++i) {
        set_debug_name((uint64_t)swapchain.images[i], VK_OBJECT_TYPE_IMAGE, std::format("Swapchain image [{}]", i));
        set_debug_name((uint64_t)swapchain.image_views[i], VK_OBJECT_TYPE_IMAGE_VIEW, std::format("Swapchain image view [{}]", i));
    }
#endif  // USE_VULKAN_DEBUG

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
    FrameData current_frame = get_current_frame();
    uint next_image_index = 0;
    vkAcquireNextImageKHR(device.device, swapchain.handle, UINT64_MAX, current_frame.swapchain_acquire_semaphore,
                          VK_NULL_HANDLE, &next_image_index);

    vkWaitForFences(device, 1, &current_frame.queue_submit_fence, VK_FALSE, UINT64_MAX);
    vkResetFences(device, 1, &current_frame.queue_submit_fence);

    VkCommandBuffer cmd = current_frame.cmd;
    VK_CHECK(vkResetCommandBuffer(cmd, 0),
             "Could not reset command buffer: Out of device memory?");

    // vkResetCommandPool ?

    const VkCommandBufferBeginInfo cmd_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info),
             "Could not begin command buffer.");
    // -- Begin recording commands ---

    transition_image(cmd, swapchain.images[next_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

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

    vkCmdBeginRendering(cmd, &rendering_info);

    vkCmdEndRendering(cmd);

    transition_image(cmd, swapchain.images[next_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // --- End recording commands ---
    vkEndCommandBuffer(cmd);

    // Submit to graphics queue
    const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &current_frame.swapchain_acquire_semaphore,
        .pWaitDstStageMask = &wait_dst_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &current_frame.swapchain_release_semaphore,
    };
    VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, current_frame.queue_submit_fence),
             "Could not submit command buffer to graphics queue");

    // Present
    const VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &current_frame.swapchain_release_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain.handle,
        .pImageIndices = &next_image_index,
        .pResults = nullptr,
    };

    const VkResult present_result = vkQueuePresentKHR(graphics_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        create_swapchain(true);
    }

    current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
}

void RendererVulkan::transition_image(VkCommandBuffer p_cmd, VkImage p_image, VkImageLayout p_current_layout, VkImageLayout p_target_layout, VkImageAspectFlags p_aspect_flags) const {
    VkImageAspectFlags aspect_mask;
    if (p_aspect_flags == VK_IMAGE_ASPECT_NONE) {
        aspect_mask = (p_target_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || p_current_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                          ? VK_IMAGE_ASPECT_DEPTH_BIT
                          : VK_IMAGE_ASPECT_COLOR_BIT;
    } else {
        aspect_mask = p_aspect_flags;
    }

    VkAccessFlagBits2 src_access_mask{};
    VkPipelineStageFlags2 src_stage_mask{};
    switch (p_current_layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            src_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
            src_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            src_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            break;
        default:;
    }

    VkAccessFlagBits2 dst_access_mask{};
    VkPipelineStageFlags2 dst_stage_mask{};
    switch (p_target_layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            dst_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
            dst_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            dst_stage_mask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            break;
        default:;
    }

    VkImageMemoryBarrier2 image_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = p_current_layout,
        .newLayout = p_target_layout,
        // srcQueueFamilyIndex
        // dstQueueFamilyIndex
        .image = p_image,
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,  // ?
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier,
    };

    vkCmdPipelineBarrier2(p_cmd, &dependency_info);
}

vkb::Instance const*
RendererVulkan::get_instance() const {
    return &instance;
}

RendererVulkan::FrameData
RendererVulkan::get_current_frame() const {
    return frames_in_flight[current_frame_index];
}

std::expected<void, std::string>
RendererVulkan::create_surface(SDL_Window* p_window) {
    if (!SDL_Vulkan_CreateSurface(p_window, instance.instance, nullptr,
                                  &surface)) [[unlikely]] {
        return std::unexpected(SDL_GetError());
    }
    return {};
}

void RendererVulkan::set_debug_name(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const {
    VkDebugUtilsObjectNameInfoEXT name_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = p_type,
        .objectHandle = p_handle,
        .pObjectName = p_name.c_str(),
    };
    vkSetDebugUtilsObjectNameEXT(device, &name_info);
}