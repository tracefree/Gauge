#include "renderer_vulkan.hpp"

#include "VkBootstrap.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <gauge/common.hpp>
#include <gauge/core/app.hpp>
#include <gauge/core/config.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/descriptor.hpp>
#include <gauge/renderer/vulkan/graphics_pipeline_builder.hpp>
#include <gauge/renderer/vulkan/imgui.hpp>
#include <gauge/renderer/vulkan/shader_module.hpp>

#include <cassert>
#include <cstdint>
#include <expected>
#include <format>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <print>
#include <string>
#include <tracy/Tracy.hpp>
#include <vector>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "gauge/components/component.hpp"
#include "gauge/core/handle.hpp"
#include "gauge/math/common.hpp"
#include "gauge/renderer/common.hpp"
#include "gauge/renderer/gltf.hpp"
#include "gauge/renderer/renderer.hpp"
#include "gauge/renderer/texture.hpp"
#include "gauge/renderer/vulkan/command_buffer.hpp"
#include "gauge/scene/node.hpp"
#include "gauge/scene/scene_tree.hpp"
#include "glm/matrix.hpp"
#include "glm/packing.hpp"
#include "thirdparty/imgui/imgui.h"

#include "thirdparty/imgui/backends/imgui_impl_sdl3.h"
#include "thirdparty/imgui/backends/imgui_impl_vulkan.h"

#include <gauge/renderer/shaders/limits.h>

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
    assert(false);
    return VK_FALSE;
}
#endif

VkSampleCountFlagBits RendererVulkan::SampleCountFromMSAA(MSAA p_msaa) {
    switch (p_msaa) {
        case MSAA::OFF:
            return VK_SAMPLE_COUNT_1_BIT;
        case MSAA::x2:
            return VK_SAMPLE_COUNT_2_BIT;
        case MSAA::x4:
            return VK_SAMPLE_COUNT_4_BIT;
        case MSAA::x8:
            return VK_SAMPLE_COUNT_8_BIT;
        case MSAA::x16:
            return VK_SAMPLE_COUNT_16_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
};

static Result<vkb::Instance>
CreateInstance(bool p_offscreen = false) {
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

    std::vector<const char*> echt = {VK_KHR_SURFACE_EXTENSION_NAME};
    instance_builder.enable_extensions(echt.size(), echt.data());

    if (p_offscreen) {
        //   instance_builder.set_headless();
    }

    auto instance_ret = instance_builder.build();
    if (!instance_ret) {
        return Error(std::format("Could not create Vulkan instance. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                 instance_ret.full_error().type.value(),
                                 instance_ret.full_error().type.message(),
                                 string_VkResult(instance_ret.full_error().vk_result)));
    }
    return instance_ret.value();
}

static Result<vkb::PhysicalDevice>
CreatePhysicalDevice(vkb::Instance p_instance, VkSurfaceKHR p_surface) {
    VkPhysicalDeviceFeatures device_features{
        .samplerAnisotropy = VK_TRUE,
    };
    VkPhysicalDeviceVulkan11Features device_features_11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .shaderDrawParameters = VK_TRUE,
    };
    VkPhysicalDeviceVulkan12Features device_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };
    VkPhysicalDeviceVulkan13Features device_features_13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };

    vkb::PhysicalDeviceSelector selector{p_instance};

    selector
        .set_minimum_version(1, 4)
        .add_required_extension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME)
        .set_required_features(device_features)
        .set_required_features_11(device_features_11)
        .set_required_features_12(device_features_12)
        .set_required_features_13(device_features_13)
        .add_required_extension(VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME)
        .add_required_extension_features<VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR>(
            VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR,
                .unifiedImageLayouts = VK_TRUE,
            });

    if (p_surface != VK_NULL_HANDLE) {
        selector.set_surface(p_surface);
    }

    auto physical_device_ret = selector.select();
    if (!physical_device_ret) {
        return Error(std::format("Could not create Vulkan physical device. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                 physical_device_ret.full_error().type.value(),
                                 physical_device_ret.full_error().type.message(),
                                 string_VkResult(physical_device_ret.full_error().vk_result)));
    }
    return physical_device_ret.value();
}

static Result<vkb::Device>
CreateDevice(vkb::PhysicalDevice p_physical_device) {
    vkb::DeviceBuilder device_builder{p_physical_device};
    auto device_ret = device_builder.build();
    if (!device_ret) {
        return Error(std::format("Could not create Vulkan logical device. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                 device_ret.full_error().type.value(),
                                 device_ret.full_error().type.message(),
                                 string_VkResult(device_ret.full_error().vk_result)));
    }
    return device_ret.value();
}

static Result<>
InitializeVolk(vkb::Instance p_instance, vkb::Device p_device) {
    VK_CHECK_RET(volkInitialize(), "Could not initialize volk");
    volkLoadInstance(p_instance.instance);
    volkLoadDevice(p_device.device);
    return {};
}

static Result<>
InitializeVulkanMemoryAllocator(VulkanContext& ctx) {
    VmaVulkanFunctions vulkan_functions;
    VmaAllocatorCreateInfo vma_allocator_info{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = ctx.physical_device.physical_device,
        .device = ctx.device.device,
        .pVulkanFunctions = &vulkan_functions,
        .instance = ctx.instance.instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,

    };
    VK_CHECK_RET(vmaImportVulkanFunctionsFromVolk(&vma_allocator_info, &vulkan_functions),
                 "Could not import vulkan functions from volk for Vulkan Memory Allocator");
    VK_CHECK_RET(vmaCreateAllocator(&vma_allocator_info, &ctx.allocator),
                 "Could not create Vulkan Memory Allocator");
    return {};
}

static Result<VkQueue>
GetQueue(vkb::Device p_device) {
    auto graphics_queue_ret = p_device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        return Error(std::format("Could not get graphics queue. vk-bootstrap error code: [{}] {}. Vulkan result: {}.",
                                 graphics_queue_ret.full_error().type.value(),
                                 graphics_queue_ret.full_error().type.message(),
                                 string_VkResult(graphics_queue_ret.full_error().vk_result)));
    }
    return graphics_queue_ret.value();
}

Result<VkCommandPool>
RendererVulkan::CreateCommandPool() const {
    const VkCommandPoolCreateInfo cmd_pool_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VkCommandPool cmd_pool;
    VK_CHECK_RET(
        vkCreateCommandPool(ctx.device, &cmd_pool_create_info, nullptr, &cmd_pool),
        "Could not create command pool");

    return cmd_pool;
}

Result<VkCommandBuffer>
RendererVulkan::CreateCommandBuffer(
    VkCommandPool p_cmd_pool) const {
    const VkCommandBufferAllocateInfo cmd_allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = p_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd;
    VK_CHECK_RET(vkAllocateCommandBuffers(ctx.device, &cmd_allocate_info, &cmd),
                 "Could not allocate command buffer");

    return cmd;
}

Result<>
RendererVulkan::CreateFrameData() {
    auto pool_result = DescriptorPool::Create(
        ctx,
        {
            {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = max_frames_in_flight},
        },
        VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
        max_frames_in_flight);
    CHECK_RET(pool_result);
    per_frame_pool = pool_result.value();

    auto layout_result =
        DescriptorSetLayoutBuilder()
            .AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
            .SetFlags(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT)
            .Build(ctx);
    CHECK_RET(layout_result);

    frames_in_flight.reserve(max_frames_in_flight);
    for (uint i = 0; i < max_frames_in_flight; i++) {
        FrameData frame{};

        // Commands
        CHECK_RET(CreateCommandPool()
                      .and_then([this, &frame](VkCommandPool p_cmd_pool) {
                          frame.cmd_pool = p_cmd_pool;
                          return CreateCommandBuffer(frame.cmd_pool);
                      })
                      .transform([&frame](VkCommandBuffer p_cmd) {
                          frame.cmd = p_cmd;
                      }));

        // Synchronization objects
        VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VK_CHECK_RET(vkCreateFence(ctx.device, &fence_info, nullptr, &frame.queue_submit_fence),
                     "Could not create render fence");

        VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VK_CHECK_RET(vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &frame.swapchain_acquire_semaphore),
                     "Could not create acquire semaphore");

        // Descriptor Set
        auto set_result = DescriptorSet::Create(ctx, layout_result.value(), per_frame_pool);
        CHECK_RET(set_result);
        frame.descriptor_set = set_result.value();

        // Uniform buffer
        CHECK_RET(
            CreateBuffer(
                sizeof(GPUGlobals),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU)
                .transform([&](GPUBuffer p_buffer) {
                    frame.uniform_buffer = p_buffer;
                }));

        frame.descriptor_set.WriteUniformBuffer(ctx, 0, 0, frame.uniform_buffer.handle, sizeof(GPUGlobals));

        // Tracy
#ifdef TRACY_ENABLE
        frame.tracy_context = TracyVkContext(ctx.physical_device, ctx.device, ctx.graphics_queue, frame.cmd);
        std::string tacy_context_name = std::format("Frame In-Flight Index {}", i);
        TracyVkContextName(frame.tracy_context, tacy_context_name.c_str(), tacy_context_name.size());
        frames_in_flight.emplace_back(frame);
#endif

        // Debug
        SetDebugName((uint64_t)frame.cmd_pool, VK_OBJECT_TYPE_COMMAND_POOL, std::format("Primary command pool [{}]", i));
        SetDebugName((uint64_t)frame.cmd, VK_OBJECT_TYPE_COMMAND_BUFFER, std::format("Primary command buffer [{}]", i));
        SetDebugName((uint64_t)frame.queue_submit_fence, VK_OBJECT_TYPE_FENCE, std::format("Queue submit fence [{}]", i));
        SetDebugName((uint64_t)frame.swapchain_acquire_semaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain acquire semaphore [{}]", i));
    }

    return {};
}

void RendererVulkan::RenderImGui(CommandBufferVulkan* cmd, uint p_next_image_index) const {
    ZoneScoped;
    TracyVkZone(GetCurrentFrame().tracy_context, cmd->GetHandle(), "ImGui");

    VkDebugUtilsLabelEXT debug_marker_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = "ImGui",
    };
    vkCmdBeginDebugUtilsLabelEXT(cmd->GetHandle(), &debug_marker_info);

    VkRenderingAttachmentInfo color_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = offscreen ? render_state.viewports[0].color.view : swapchain.image_views[p_next_image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .flags = 0,
        .renderArea =
            {
                .offset = {.x = 0, .y = 0},
                .extent =
                    {
                        .width = window_size.width,  // TODO
                        .height = window_size.height,
                    },
            },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
    };
    vkCmdBeginRendering(cmd->GetHandle(), &rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd->GetHandle());
    vkCmdEndRendering(cmd->GetHandle());

    vkCmdEndDebugUtilsLabelEXT(cmd->GetHandle());
}

Result<>
RendererVulkan::CreateSwapchain(bool recreate) {
    if (offscreen) {
        return {};
    }
    vkb::SwapchainBuilder swapchain_builder{ctx.device};
    if (recreate) {
        swapchain_builder.set_old_swapchain(swapchain.vkb_swapchain);
        vkDeviceWaitIdle(ctx.device);
    }
    auto swapchain_ret =
        swapchain_builder
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(window_size.width, window_size.height)
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_desired_min_image_count(3)
            .build();
    if (!swapchain_ret) {
        swapchain.handle = VK_NULL_HANDLE;
        return Error(std::format("Could not create swapchain. vk-bootstrap error code: [{}] {}. Vulkan result: {}",
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
        vkDestroySemaphore(ctx.device, old_semaphore, nullptr);
    }
    swapchain_release_semaphores.resize(swapchain.vkb_swapchain.image_count);

    VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (uint i = 0; i < swapchain.vkb_swapchain.image_count; ++i) {
        VK_CHECK_RET(vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &swapchain_release_semaphores[i]),
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

Result<VkSampler>
RendererVulkan::CreateSampler(VkFilter p_filter_mode) const {
    VkSampler sampler{};
    VkSamplerCreateInfo sampler_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = p_filter_mode,
        .minFilter = p_filter_mode,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    };
    VK_CHECK_RET(vkCreateSampler(ctx.device, &sampler_info, nullptr, &sampler),
                 "Could not create sampler");
    return sampler;
}

Result<Pipeline>
RendererVulkan::CreateGraphicsPipeline(std::string p_name) {
    auto shader_module_result = ShaderModule::FromFile(ctx, "simple.spv");
    CHECK_RET(shader_module_result);
    ShaderModule shader_module = shader_module_result.value();

    GraphicsPipelineBuilder builder("simple");
    return builder
        .SetVertexStage(shader_module.handle, "VertexMain")
        .SetFragmentStage(shader_module.handle, "FragmentMain")
        .AddDescriptorSetLayout(global_descriptor.layout)
        .AddDescriptorSetLayout(frames_in_flight[0].descriptor_set.GetLayout())
        .AddPushConstantRange((VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), sizeof(PushConstants))
        .SetImageFormat(offscreen ? VK_FORMAT_R8G8B8A8_SRGB : swapchain.image_format)
        .SetSampleCount(SampleCountFromMSAA(gApp->project_settings.msaa_level))
        .Build(ctx);
}

Result<> RendererVulkan::InitializeGlobalResources() {
    uint white = glm::packUnorm4x8(Vec4(1.0f));
    resources.texture_white = CreateTexture({
        .data = (unsigned char*)&white,
        .width = 1,
        .height = 1,
        .use_srgb = false,
    });

    uint black = glm::packUnorm4x8(Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    resources.texture_black = CreateTexture({
        .data = (unsigned char*)&black,
        .width = 1,
        .height = 1,
        .use_srgb = false,
    });

    uint normal = glm::packUnorm4x8(Vec4(0.5f, 0.5f, 1.0f, 1.0f));
    resources.texture_normal = CreateTexture({
        .data = (unsigned char*)&normal,
        .width = 1,
        .height = 1,
        .use_srgb = false,
    });

    uint magenta = glm::packUnorm4x8(Vec4(1.0f, 0.0f, 1.0f, 1.0f));
    std::array<uint32_t, 16 * 16> missing;
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            missing[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    resources.texture_missing = CreateTexture({
        .data = (unsigned char*)&missing,
        .width = 16,
        .height = 16,
        .use_srgb = false,
    });

    Result<GPUBuffer> buffer_result =
        CreateBuffer(
            sizeof(GPUMaterial) * MAX_DESCRIPTOR_SETS,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
    CHECK_RET(buffer_result);
    resources.materials_buffer = buffer_result.value();
    global_descriptor.set.WriteStorageBuffer(ctx, 2, 0, resources.materials_buffer.handle, sizeof(GPUMaterial) * MAX_DESCRIPTOR_SETS);

    render_state.camera_views.resize(MAX_CAMERAS);
    render_state.camera_projections.resize(MAX_CAMERAS);
    render_state.camera_view_projections.resize(MAX_CAMERAS);

    return {};
}

Result<>
RendererVulkan::Initialize(void (*p_create_surface)(VkInstance p_instance, VkSurfaceKHR* r_surface), bool p_offscreen) {
    offscreen = p_offscreen;
    CHECK_RET(
        CreateInstance(offscreen)
            .transform([&](vkb::Instance p_instance) {
                ctx.instance = p_instance;
            }));

    p_create_surface(ctx.instance, &surface);

    window_size = {.width = 1920, .height = 1080};

    CHECK_RET(
        CreatePhysicalDevice(ctx.instance, surface)
            .and_then([&](vkb::PhysicalDevice p_physical_device) {
                ctx.physical_device = p_physical_device;
                return CreateDevice(p_physical_device);
            })
            .and_then([&](vkb::Device p_device) {
                ctx.device = p_device;
                return InitializeVolk(ctx.instance, ctx.device);
            })
            .and_then([&]() {
                return InitializeVulkanMemoryAllocator(ctx);
            })
            .transform([&]() {
                SetDebugName((uint64_t)ctx.instance.instance, VK_OBJECT_TYPE_INSTANCE, "Primary instance");
                SetDebugName((uint64_t)ctx.physical_device.physical_device, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "Primary physical device");
                SetDebugName((uint64_t)ctx.device.device, VK_OBJECT_TYPE_DEVICE, "Primary device");
                SetDebugName((uint64_t)surface, VK_OBJECT_TYPE_SURFACE_KHR, "Main window surface");
            })
            .and_then([&]() {
                ctx.graphics_queue_family_index = ctx.device.get_queue_index(vkb::QueueType::graphics).value();
                return GetQueue(ctx.device);
            })
            .and_then([&](VkQueue p_queue) {
                ctx.graphics_queue = p_queue;
                SetDebugName((uint64_t)ctx.graphics_queue, VK_OBJECT_TYPE_QUEUE, "Graphics queue");

                return CreateFrameData();
            })
            .and_then([&]() {
                return CreateSwapchain();
            })
            .and_then([&]() {
                return CreateSampler(VK_FILTER_LINEAR);
            })
            .and_then([&](VkSampler p_linear_filter) {
                samplers.linear = p_linear_filter;

                return CreateSampler(VK_FILTER_NEAREST);
            })
            .and_then([&](VkSampler p_nearest_filter) {
                samplers.nearest = p_nearest_filter;

                return DescriptorPool::Create(
                    ctx,
                    {
                        {
                            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                            .descriptorCount = 2,
                        },
                        {
                            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                            .descriptorCount = MAX_DESCRIPTOR_SETS,
                        },
                        {
                            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                            .descriptorCount = 1,
                        },
                    },
                    VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
                    MAX_DESCRIPTOR_SETS);
            })
            .and_then([&](VkDescriptorPool p_pool) {
                global_descriptor.pool = p_pool;

                VkSampler immputable_samplers[] = {samplers.linear, samplers.nearest};
                return DescriptorSetLayoutBuilder()
                    .AddBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 2, VK_SHADER_STAGE_ALL, immputable_samplers)
                    .AddBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_DESCRIPTOR_SETS)
                    .AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
                    .Build(ctx);
            })
            .and_then([&](VkDescriptorSetLayout p_layout) {
                global_descriptor.layout = p_layout;

                return DescriptorSet::Create(ctx, global_descriptor.layout, global_descriptor.pool);
            })
            .and_then([&](DescriptorSet p_set) {
                global_descriptor.set = p_set;
                return InitializeImGui(*this);
            }));

    const auto graphics_pipeline_result = CreateGraphicsPipeline("Primary graphics pipeline");
    CHECK_RET(graphics_pipeline_result);
    graphics_pipeline = graphics_pipeline_result.value();

    // Immediate command setup
    CHECK_RET(
        CreateCommandPool()
            .and_then([&](VkCommandPool p_cmd_pool) {
                immediate_command.pool = p_cmd_pool;
                return CreateCommandBuffer(immediate_command.pool);
            })
            .transform([&](VkCommandBuffer p_cmd) {
                immediate_command.buffer = p_cmd;
            }));
    const VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VK_CHECK_RET(vkCreateFence(ctx.device, &fence_info, nullptr, &immediate_command.fence),
                 "Could not immediate submit fence");

    CHECK_RET(InitializeGlobalResources());

    // VMA Pool
    {
        const VkImageCreateInfo dummy_image_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent = VkExtent3D{.width = 1920, .height = 1080, .depth = 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        };
        const VmaAllocationCreateInfo dummy_allocation_info{
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        uint memory_type_index{};
        vmaFindMemoryTypeIndexForImageInfo(ctx.allocator, &dummy_image_info, &dummy_allocation_info, &memory_type_index);

        VkExportMemoryAllocateInfo export_memory{
            .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
        };
        const VmaPoolCreateInfo pool_info{
            .memoryTypeIndex = memory_type_index,
            .pMemoryAllocateNext = &export_memory,
        };
        VK_CHECK_RET(vmaCreatePool(ctx.allocator, &pool_info, &external_pool),
                     "Could not create VMA pool for exportable images");
    }

    // Render state
    const auto main_viewport_result =
        CreateViewport(ViewportSettings{
            .width = static_cast<float>(window_size.width),
            .height = static_cast<float>(window_size.height),
            .msaa = gApp->project_settings.msaa_level,
            .fill_window = true,
            .use_swapchain = false,
            .use_depth = true,
        });
    CHECK_RET(main_viewport_result)
    render_state.viewports.emplace_back(main_viewport_result.value());
    render_state.viewports[0].scene_tree = std::make_shared<SceneTree>();
    render_state.viewports[0].scene_tree->root = std::make_shared<Node>();

    initialized = true;
    return {};
}

void RendererVulkan::RenderViewport(const CommandBufferVulkan& cmd, const Viewport& p_viewport, uint p_next_image_index) {
    TracyVkZone(GetCurrentFrame().tracy_context, cmd.GetHandle(), "Viewport");

    const bool draw_to_swapchain = p_viewport.settings.use_swapchain && p_viewport.settings.render_scale == 1.0f;
    const VkImageView target_view = draw_to_swapchain ? swapchain.image_views[p_next_image_index] : p_viewport.color.view;
    const int scaled_width = (int)(p_viewport.settings.width * p_viewport.settings.render_scale);
    const int scaled_height = (int)(p_viewport.settings.height * p_viewport.settings.render_scale);

    if (!draw_to_swapchain) {
        cmd.TransitionImage(p_viewport.color.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    }
    VkRenderingAttachmentInfo color_attachement_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = target_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {
            .color = {{0.01f, 0.01f, 0.01f, 0.0f}},
        }};

    if (p_viewport.settings.msaa != MSAA::OFF) {
        color_attachement_info.imageView = p_viewport.color_multisampled.view;
        color_attachement_info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        color_attachement_info.resolveImageView = target_view;
        color_attachement_info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .flags = 0,
        .renderArea =
            {
                .offset = {.x = 0, .y = 0},
                .extent =
                    {
                        .width = (uint)scaled_width,
                        .height = (uint)scaled_height,
                    },
            },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachement_info,
    };

    if (p_viewport.settings.use_depth) {
        VkRenderingAttachmentInfo depth_attachement_info{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = p_viewport.depth.view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .depthStencil = {.depth = 0.0f},
            }};
        rendering_info.pDepthAttachment = &depth_attachement_info;

        if (p_viewport.settings.msaa != MSAA::OFF) {
            depth_attachement_info.imageView = p_viewport.depth_multisampled.view;
            depth_attachement_info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            depth_attachement_info.resolveImageView = p_viewport.depth.view;
            depth_attachement_info.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        }
    }

    vkCmdBeginRendering(cmd.GetHandle(), &rendering_info);

    const VkViewport vk_viewport{
        p_viewport.settings.position.x, p_viewport.settings.position.y,
        (float)scaled_width, (float)scaled_height,
        0.0f, 1.0f};
    const VkExtent2D extent = {p_viewport.color.extent.width, p_viewport.color.extent.height};
    const VkRect2D scissor{
        VkOffset2D{}, p_viewport.settings.use_swapchain ? swapchain.extent : extent};
    vkCmdSetViewport(cmd.GetHandle(), 0, 1, &vk_viewport);
    vkCmdSetScissor(cmd.GetHandle(), 0, 1, &scissor);

    cmd.BindPipeline(graphics_pipeline);
    PushConstants& pcs = GetCurrentFrame().push_constants;
    pcs.sampler = linear ? 0 : 1;

    draw_objects.clear();
    p_viewport.scene_tree->root->RefreshTransform();
    p_viewport.scene_tree->Draw();

    for (const DrawObject& draw_object : draw_objects) {
        pcs.model_matrix = draw_object.transform.GetMatrix();
        const GPUMesh& mesh = *resources.meshes.Get(draw_object.primitive);
        pcs.vertex_buffer_address = mesh.vertex_buffer.address;
        pcs.material_index = draw_object.material.index;
        pcs.camera_index = 0;
        vkCmdPushConstants(cmd.GetHandle(), graphics_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);
        vkCmdBindIndexBuffer(cmd.GetHandle(), mesh.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd.GetHandle(), mesh.index_count, 1, 0, 0, 0);
    }

    for (auto callback : render_state.render_callbacks) {
        callback(ctx, cmd);
    }

    vkCmdEndRendering(cmd.GetHandle());

    cmd.TransitionImage(p_viewport.color.handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);

    if (!draw_to_swapchain && !offscreen) {
        cmd.TransitionImage(p_viewport.color.handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        cmd.TransitionImage(swapchain.images[p_next_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        const VkImageBlit image_blit = {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .srcOffsets = {
                {.x = 0, .y = 0, .z = 0},
                {.x = (int)p_viewport.color.extent.width, .y = (int)p_viewport.color.extent.height, .z = 1},
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .dstOffsets = {
                {.x = 0, .y = 0, .z = 0},
                {.x = (int)(swapchain.extent.width), .y = (int)(swapchain.extent.height), .z = 1},
            }};
        vkCmdBlitImage(
            cmd.GetHandle(),
            p_viewport.color.handle,
            VK_IMAGE_LAYOUT_GENERAL,
            swapchain.images[p_next_image_index],
            VK_IMAGE_LAYOUT_GENERAL,
            1,
            &image_blit,
            VK_FILTER_LINEAR);
        cmd.TransitionImage(swapchain.images[p_next_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    }
}

void RendererVulkan::RecordCommands(const CommandBufferVulkan& cmd, uint p_next_image_index) {
    ZoneScoped;
    TracyVkZone(GetCurrentFrame().tracy_context, cmd.GetHandle(), "Draw");

    if (!offscreen) {
        cmd.TransitionImage(swapchain.images[p_next_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    }

    // Update uniform buffer
    // TODO: Move elsewhere
    const auto current_time = std::chrono::steady_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - gApp->start_time).count();
    GPUGlobals global_uniforms{
        .time = time,
    };
    for (uint i = 0; i < render_state.viewports.size(); ++i) {
        const auto& viewport = render_state.viewports[i];

        Mat4 projection = glm::perspective(
            glm::radians(viewport.field_of_view),
            (float)(viewport.settings.width / viewport.settings.height),
            100.0f,
            0.1f);
        projection[1][1] *= -1.0f;

        render_state.camera_projections[i] = projection;
        render_state.camera_view_projections[i] = projection * render_state.camera_views[i];

        global_uniforms.cameras[i] = GPUCamera{
            .view = render_state.camera_views[i],
            .view_projection = render_state.camera_view_projections[i],
            .inverse_projection = glm::inverse(projection),
        };
    }
    memcpy(GetCurrentFrame().uniform_buffer.allocation.info.pMappedData, &global_uniforms, sizeof(GPUGlobals));

    const VkDescriptorSet sets[] = {
        global_descriptor.set.handle,
        GetCurrentFrame().descriptor_set.handle,
    };
    vkCmdBindDescriptorSets(cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 0, 2, sets, 0, nullptr);

    // Render
    for (const auto& viewport : render_state.viewports) {
        RenderViewport(cmd, viewport, p_next_image_index);
    }

    // RenderImGui(cmd, p_next_image_index);

    if (!offscreen) {
        cmd.TransitionImage(swapchain.images[p_next_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
}

static void NodeTree(const Ref<Node>& node) {
    bool open = ImGui::TreeNodeEx(node->name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap);
    ImGui::SameLine(ImGui::GetWindowWidth() - 30);
    ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 0.0f);
    ImGui::Checkbox(std::format("##{}_vis", node->name.c_str()).c_str(), &node->visible);
    ImGui::PopStyleVar();
    if (open) {
        ImGui::Text("Position");
        ImGui::InputFloat("x", &node->local_transform.position.x, 0.01f, 0.1f, "%.3f");
        ImGui::InputFloat("y", &node->local_transform.position.y, 0.01f, 0.1f, "%.3f");
        ImGui::InputFloat("z", &node->local_transform.position.z, 0.01f, 0.1f, "%.3f");

        ImGui::InputFloat("Scale", &node->local_transform.scale, 0.01f, 0.1f, "%.3f");

        for (auto const& component : node->GetComponents()) {
            bool is_component_open = ImGui::TreeNode("MeshInstance");
            ImGui::SameLine(ImGui::GetWindowWidth() - 30);
            ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 0.0f);
            ImGui::Checkbox(std::format("##{}_comp_vis", node->name.c_str()).c_str(), &component->visible);
            ImGui::PopStyleVar();
            if (is_component_open) {
                ImGui::TreePop();
            }
        }
        for (const Ref<Node>& child : node->children) {
            NodeTree(child);
        }
        ImGui::TreePop();
    }
};

void RendererVulkan::Draw() {
    ZoneScoped;
    if (false) {
        ZoneScopedN("ImGui calls");
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Settings")) {
            ImGui::Checkbox("Show albedo", &linear);
            if (ImGui::SliderFloat("Render scale", &render_state.viewports[0].settings.render_scale, 0.1f, 1.0f)) {
                OnViewportResized(render_state.viewports[0], render_state.viewports[0].settings.width, render_state.viewports[0].settings.height);
            }
        }
        ImGui::End();

        if (ImGui::Begin("Scene Tree")) {
            for (auto& node : render_state.viewports[0].scene_tree->root->children) {
                NodeTree(node);
            }
        }
        ImGui::End();

        ImGui::Render();
    }

    const FrameData& current_frame = GetCurrentFrame();
    uint next_image_index = 0;
    {
        ZoneScopedN("vkWaitForFences");
        while (vkWaitForFences(ctx.device, 1, &current_frame.queue_submit_fence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT)
            ;
    }
    {
        ZoneScopedN("vkAcquireNextImage");
        // TODO: Check result value and recreate swapchain if necessary
        vkAcquireNextImageKHR(ctx.device.device, swapchain.handle, UINT64_MAX, current_frame.swapchain_acquire_semaphore,
                              VK_NULL_HANDLE, &next_image_index);
    }
    VK_CHECK(vkResetFences(ctx.device, 1, &current_frame.queue_submit_fence),
             "Could not reset queue submit fence");
    VK_CHECK(vkResetCommandPool(ctx.device, current_frame.cmd_pool, 0),
             "Could not reset command pool");

    const VkCommandBuffer current_command_buffer = current_frame.cmd;
    CommandBufferVulkan cmd{current_command_buffer};

    CHECK(cmd.Begin());
    RecordCommands(cmd, next_image_index);
    TracyVkCollect(current_frame.tracy_context, cmd.GetHandle());
    CHECK(cmd.End());
    {
        ZoneScopedN("vkQueueSubmit");
        // Submit to graphics queue
        const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &current_frame.swapchain_acquire_semaphore,
            .pWaitDstStageMask = &wait_dst_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &current_command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &swapchain_release_semaphores[next_image_index],
        };
        VK_CHECK(vkQueueSubmit(ctx.graphics_queue, 1, &submit_info, current_frame.queue_submit_fence),
                 "Could not submit command buffer to graphics queue");
    }

    VkResult present_result;
    {
        ZoneScopedN("vkQueuePresentKHR");
        // Present
        const VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &swapchain_release_semaphores[next_image_index],
            .swapchainCount = 1,
            .pSwapchains = &swapchain.handle,
            .pImageIndices = &next_image_index,
            .pResults = nullptr,
        };
        present_result = vkQueuePresentKHR(ctx.graphics_queue, &present_info);
    }

    FrameMark;
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        CHECK(CreateSwapchain(true));
    }

    current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
}

void RendererVulkan::DrawOffscreen() {
    FrameData current_frame = GetCurrentFrame();
    VkCommandBuffer current_command_buffer = current_frame.cmd;
    CommandBufferVulkan cmd{current_command_buffer};

    CHECK(cmd.Begin());
    RecordCommands(cmd, 0);
    TracyVkCollect(current_frame.tracy_context, cmd.GetHandle());
    CHECK(cmd.End());
    {
        ZoneScopedN("vkQueueSubmit");
        // Submit to graphics queue
        const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pWaitDstStageMask = &wait_dst_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &current_command_buffer,
        };
        VK_CHECK(vkQueueSubmit(ctx.graphics_queue, 1, &submit_info, VK_NULL_HANDLE),
                 "Could not submit command buffer to graphics queue");
    }
    FrameMark;

    current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
}

RendererVulkan::FrameData const&
RendererVulkan::GetCurrentFrame() const {
    return frames_in_flight[current_frame_index];
}

RendererVulkan::FrameData&
RendererVulkan::GetCurrentFrame() {
    return frames_in_flight[current_frame_index];
}

void RendererVulkan::SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const {
#ifdef USE_VULKAN_DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = p_type,
        .objectHandle = p_handle,
        .pObjectName = p_name.c_str(),
    };
    vkSetDebugUtilsObjectNameEXT(ctx.device, &name_info);
#endif  // USE_VULKAN_DEBUG
}

void RendererVulkan::OnWindowResized(uint p_width, uint p_height) {
    window_size.width = p_width;
    window_size.height = p_height;
    for (auto& viewport : render_state.viewports) {
        if (viewport.settings.fill_window) {
            OnViewportResized(viewport, p_width, p_height);
        }
    }
    CHECK(CreateSwapchain(true));
}

void RendererVulkan::OnViewportResized(Viewport& p_viewport, uint p_width, uint p_height) const {
    vkDeviceWaitIdle(ctx.device);

    p_viewport.settings.width = p_width;
    p_viewport.settings.height = p_height;
    if (p_viewport.settings.use_depth) {
        ViewportDestroyImages(p_viewport);
        ViewportCreateImages(p_viewport);
    }
}

Result<GPUBuffer>
RendererVulkan::CreateBuffer(size_t p_allocation_size, VkBufferUsageFlags p_usage, VmaMemoryUsage p_memory_usage) const {
    GPUBuffer buffer{};

    const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = p_allocation_size,
        .usage = p_usage,
    };
    const VmaAllocationCreateInfo vma_alloc_info{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        .usage = p_memory_usage,
    };

    VK_CHECK_RET(
        vmaCreateBuffer(
            ctx.allocator,
            &buffer_info,
            &vma_alloc_info,
            &buffer.handle,
            &buffer.allocation.handle,
            &buffer.allocation.info),
        "Could not create buffer")

    return buffer;
}

Result<GPUMesh>
RendererVulkan::UploadMeshToGPU(const std::vector<Vertex>& p_vertices, const std::vector<uint>& p_indices) const {
    GPUMesh gpu_mesh{};
    const uint vertex_buffer_size = p_vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = p_indices.size() * sizeof(uint);

    gpu_mesh.index_count = p_indices.size();

    // Vertices
    const auto vertex_buffer_result = CreateBuffer(
        vertex_buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    CHECK_RET(vertex_buffer_result);
    gpu_mesh.vertex_buffer = vertex_buffer_result.value();
    const VkBufferDeviceAddressInfo vertex_buffer_address_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = gpu_mesh.vertex_buffer.handle,
    };
    gpu_mesh.vertex_buffer.address = vkGetBufferDeviceAddress(ctx.device, &vertex_buffer_address_info);

    // Indices
    const auto index_buffer_result = CreateBuffer(
        index_buffer_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    CHECK_RET(index_buffer_result);
    gpu_mesh.index_buffer = index_buffer_result.value();

    // Staging
    GPUBuffer staging_buffer{};
    const auto staging_buffer_result = CreateBuffer(
        (vertex_buffer_size + index_buffer_size),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_COPY);
    CHECK_RET(staging_buffer_result);
    staging_buffer = staging_buffer_result.value();

    void* data = staging_buffer.allocation.info.pMappedData;
    memcpy(data, p_vertices.data(), vertex_buffer_size);
    memcpy((char*)data + vertex_buffer_size, p_indices.data(), index_buffer_size);

    ImmediateSubmit([&](CommandBufferVulkan cmd) {
        const VkBufferCopy vertex_copy{
            .size = vertex_buffer_size,
        };
        vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, gpu_mesh.vertex_buffer.handle, 1, &vertex_copy);

        const VkBufferCopy index_copy{
            .srcOffset = vertex_buffer_size,
            .size = index_buffer_size,
        };
        vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, gpu_mesh.index_buffer.handle, 1, &index_copy);
    });

    vmaDestroyBuffer(ctx.allocator, staging_buffer.handle, staging_buffer.allocation.handle);

    return gpu_mesh;
}

Result<GPUMesh>
RendererVulkan::UploadMeshToGPU(const glTF::Primitive& primitive) const {
    return UploadMeshToGPU(primitive.vertices, primitive.indices);
}

Result<GPUImage>
RendererVulkan::CreateImage(
    VkExtent3D p_size,
    VkFormat p_format,
    VkImageUsageFlags p_usage,
    bool p_mipmapped,
    VkSampleCountFlagBits p_sample_count,
    VkImageAspectFlagBits p_aspect_flags,
    bool p_exported) const {
    GPUImage image{
        .format = p_format,
        .extent = p_size,
    };

    const VkExternalMemoryImageCreateInfo external_memory_info{
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
    };

    const VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = p_exported ? &external_memory_info : nullptr,
        .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = image.format,
        .extent = image.extent,
        .mipLevels = p_mipmapped ? (static_cast<uint32_t>(std::floor(std::log2(std::max(p_size.width, p_size.height)))) + 1) : 1,
        .arrayLayers = 1,
        .samples = p_sample_count,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = p_usage,
    };
    const VmaAllocationCreateInfo image_allocation_info{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .pool = p_exported ? external_pool : nullptr,
    };
    VK_CHECK_RET(vmaCreateImage(ctx.allocator, &image_info, &image_allocation_info, &image.handle, &image.allocation.handle, &image.allocation.info),
                 "Could not create image");

    const VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = image.format,
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask = p_aspect_flags,
            .baseMipLevel = 0,
            .levelCount = image_info.mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    VK_CHECK(vkCreateImageView(ctx.device, &view_info, nullptr, &image.view),
             "Could not create image view");

    if (p_exported) {
        const VkMemoryGetFdInfoKHR handle_info{
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
            .memory = image.allocation.info.deviceMemory,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
        };
        VK_CHECK_RET(
            vkGetMemoryFdKHR(ctx.device, &handle_info, &image.file_descriptor),
            "Could not get memory file descriptor");
    }

    return image;
}

Result<GPUImage>
RendererVulkan::UploadTextureToGPU(const Texture& p_texture) const {
    GPUImage image{};
    GPUBuffer staging_buffer{};
    const VkExtent3D image_extent = {.width = p_texture.width, .height = p_texture.height, .depth = 1};
    return CreateImage(
               image_extent,
               p_texture.use_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .and_then([&](GPUImage p_image) {
            image = p_image;
            return CreateBuffer(
                p_texture.GetSize(),
                VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        })
        .and_then([&](GPUBuffer p_staging_buffer) {
            staging_buffer = p_staging_buffer;
            memcpy(staging_buffer.allocation.info.pMappedData, p_texture.data, p_texture.GetSize());
            return ImmediateSubmit([&](CommandBufferVulkan cmd) {
                cmd.TransitionImage(
                    image.handle,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL);
                const VkBufferImageCopy buffer_image_copy{
                    .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .layerCount = 1,
                    },
                    .imageExtent = image_extent};
                vkCmdCopyBufferToImage(
                    cmd.GetHandle(),
                    staging_buffer.handle,
                    image.handle,
                    VK_IMAGE_LAYOUT_GENERAL,
                    1, &buffer_image_copy);
                cmd.TransitionImage(
                    image.handle,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_GENERAL);
            });
        })
        .and_then([&]() -> Result<GPUImage> {
            vmaDestroyBuffer(ctx.allocator, staging_buffer.handle, staging_buffer.allocation.handle);
            return image;
        });
}

Result<>
RendererVulkan::ImmediateSubmit(std::function<void(CommandBufferVulkan p_cmd)>&& function) const {
    vkResetFences(ctx.device, 1, &immediate_command.fence);
    vkResetCommandPool(ctx.device, immediate_command.pool, 0);

    const VkCommandBufferBeginInfo cmd_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK_RET(vkBeginCommandBuffer(immediate_command.buffer, &cmd_begin_info),
                 "Could not begin immediate command buffer");

    function(immediate_command.buffer);

    VK_CHECK_RET(vkEndCommandBuffer(immediate_command.buffer),
                 "Could not end immediate command buffer");

    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &immediate_command.buffer,
    };
    VK_CHECK_RET(vkQueueSubmit(ctx.graphics_queue, 1, &submit_info, immediate_command.fence),
                 "Could not submit immediate command");
    VK_CHECK_RET(vkWaitForFences(ctx.device, 1, &immediate_command.fence, VK_TRUE, UINT64_MAX),
                 "Could not wait for immediate submit fence");

    return {};
}

Result<GPUImage>
RendererVulkan::CreateDepthImage(const uint p_width, const uint p_height, VkSampleCountFlagBits p_sample_count) const {
    return CreateImage(
               {p_width, p_height, 1},
               VK_FORMAT_D32_SFLOAT,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               false,
               p_sample_count,
               VK_IMAGE_ASPECT_DEPTH_BIT)
        .transform([&](GPUImage depth) {
            ImmediateSubmit([&depth](CommandBufferVulkan cmd) {
                cmd.TransitionImage(
                    depth.handle,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
            });
            return depth;
        });
}

void RendererVulkan::DestroyImage(GPUImage& p_image) const {
    vkDestroyImageView(ctx.device, p_image.view, nullptr);
    vmaDestroyImage(ctx.allocator, p_image.handle, p_image.allocation.handle);
    p_image.handle = VK_NULL_HANDLE;
    p_image.view = VK_NULL_HANDLE;
    p_image.format = VK_FORMAT_UNDEFINED;
    p_image.extent = {};
}

Result<RendererVulkan::Viewport> RendererVulkan::CreateViewport(const ViewportSettings& p_settings) const {
    Viewport viewport{
        .settings = p_settings,
    };

    return ViewportCreateImages(viewport)
        .transform([&viewport]() {
            return viewport;
        });
}

void RendererVulkan::ViewportDestroyImages(Viewport& p_viewport) const {
    if (p_viewport.settings.msaa != MSAA::OFF) {
        DestroyImage(p_viewport.color_multisampled);
        if (p_viewport.settings.use_depth) {
            DestroyImage(p_viewport.depth_multisampled);
        }
    }
    if (p_viewport.color.handle != VK_NULL_HANDLE) {
        DestroyImage(p_viewport.color);
    }
    if (p_viewport.settings.use_depth) {
        DestroyImage(p_viewport.depth);
    }
}

Result<> RendererVulkan::ViewportCreateImages(Viewport& p_viewport) const {
    const float render_scale = std::clamp(p_viewport.settings.render_scale, 0.1f, 1.0f);
    const uint scaled_width = (uint)(p_viewport.settings.width * render_scale);
    const uint scaled_height = (uint)(p_viewport.settings.height * render_scale);
    if (p_viewport.settings.msaa != MSAA::OFF) {
        CHECK_RET(
            CreateImage(
                {.width = scaled_width, .height = scaled_height, .depth = 1},
                offscreen ? VK_FORMAT_R8G8B8A8_SRGB : swapchain.image_format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                false, (VkSampleCountFlagBits)p_viewport.settings.msaa)
                .transform([&](GPUImage p_color_multisampled) {
                    p_viewport.color_multisampled = p_color_multisampled;
                }));
        if (p_viewport.settings.use_depth) {
            CHECK_RET(
                CreateDepthImage(
                    scaled_width,
                    scaled_height,
                    SampleCountFromMSAA(p_viewport.settings.msaa))
                    .transform([&](GPUImage p_depth_multisampled) {
                        p_viewport.depth_multisampled = p_depth_multisampled;
                    }));
        }
    }

    if (!p_viewport.settings.use_swapchain || p_viewport.settings.render_scale < 1.0f) {
        CHECK_RET(
            CreateImage(
                {.width = scaled_width, .height = scaled_height, .depth = 1},
                offscreen ? VK_FORMAT_R8G8B8A8_SRGB : swapchain.image_format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                false,
                VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                false)
                .transform([&](GPUImage p_color) {
                    p_viewport.color = p_color;
                }));
    }
    if (p_viewport.settings.use_depth) {
        CHECK_RET(
            CreateDepthImage(scaled_width, scaled_height)
                .transform([&](GPUImage p_depth) {
                    p_viewport.depth = p_depth;
                }));
    }
    return {};
}

Handle<GPUMesh> RendererVulkan::CreateMesh(std::vector<Vertex> p_vertices, std::vector<uint> p_indices) {
    Handle<GPUMesh> handle{};
    CHECK(UploadMeshToGPU(p_vertices, p_indices)
              .transform([&](GPUMesh p_mesh) {
                  handle = resources.meshes.Allocate(p_mesh);
              }));
    return handle;
}

void RendererVulkan::DestroyMesh(Handle<GPUMesh> p_handle) {
    // TODO
}

Handle<GPUImage> RendererVulkan::CreateTexture(const Texture& p_texture) {
    Handle<GPUImage> handle{};
    Result<GPUImage> image_result =
        UploadTextureToGPU(p_texture)
            .transform([&](GPUImage p_image) {
                handle = resources.textures.Allocate(p_image);
                return p_image;
            });
    CHECK(image_result);
    if (!image_result) {
        return handle;
    }
    global_descriptor.set.WriteImage(ctx, 1, (uint)handle.index, image_result->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return handle;
}

void RendererVulkan::DestroyTexture(Handle<GPUImage> p_handle) {
    // TODO
}

Handle<GPUMaterial> RendererVulkan::CreateMaterial(const GPUMaterial& p_material) {
    Handle<GPUMaterial> handle = resources.materials.Allocate(p_material);

    // Staging
    const auto staging_buffer_result = CreateBuffer(
        sizeof(GPUMaterial),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_COPY);
    CHECK(staging_buffer_result);
    GPUBuffer staging_buffer = staging_buffer_result.value();

    void* data = staging_buffer.allocation.info.pMappedData;
    memcpy(data, &p_material, sizeof(GPUMaterial));

    ImmediateSubmit([&](CommandBufferVulkan cmd) {
        const VkBufferCopy buffer_copy{
            .dstOffset = (handle.index) * sizeof(GPUMaterial),
            .size = sizeof(GPUMaterial),
        };
        vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, resources.materials_buffer.handle, 1, &buffer_copy);
    });

    vmaDestroyBuffer(ctx.allocator, staging_buffer.handle, staging_buffer.allocation.handle);
    return handle;
}

void RendererVulkan::DestroyMaterial(RID p_rid) {
    // TODO
}

void RendererVulkan::OnShaderChanged() {
    vkDeviceWaitIdle(ctx.device);
    vkDestroyPipeline(ctx.device, graphics_pipeline.handle, nullptr);
    vkDestroyPipelineLayout(ctx.device, graphics_pipeline.layout, nullptr);
    graphics_pipeline = CreateGraphicsPipeline("Primary graphics pipeline").value();
}

void RendererVulkan::ViewportSetCameraView(uint p_viewport_id, const Mat4& p_view) {
    render_state.camera_views[p_viewport_id] = p_view;
}

void RendererVulkan::ViewportSetCameraPosition(uint p_viewport_id, const Vec3& p_position) {
    render_state.viewports[p_viewport_id].camera_position = p_position;
}

void RendererVulkan::ViewportMoveCamera(uint p_viewport_id, const Vec3& p_offset) {
    render_state.viewports[p_viewport_id].camera_position += p_offset;
}

void RendererVulkan::ViewportRotateCamera(uint p_viewport_id, float p_yaw, float p_pitch) {
    Viewport& viewport = render_state.viewports[p_viewport_id];
    viewport.camera_pitch = std::clamp(viewport.camera_pitch + p_pitch, -M_PI_2f, M_PI_2f);
    viewport.camera_yaw = viewport.camera_yaw + p_yaw;
}

Quaternion RendererVulkan::ViewportGetCameraRotation(uint p_viewport_id) {
    const Viewport& viewport = render_state.viewports[p_viewport_id];
    return glm::angleAxis(viewport.camera_yaw, Vec3(0.0f, -1.0f, 0.0f)) *
           glm::angleAxis(viewport.camera_pitch, Vec3(1.0f, 0.0f, 0.0f));
}