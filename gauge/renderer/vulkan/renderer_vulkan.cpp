#include "renderer_vulkan.hpp"

#include "VkBootstrap.h"

#include <gauge/common.hpp>
#include <gauge/core/app.hpp>
#include <gauge/core/math.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/graphics_pipeline_builder.hpp>
#include <gauge/renderer/vulkan/pipeline.hpp>
#include <gauge/renderer/vulkan/shader_module.hpp>

#include <cassert>
#include <cstdint>
#include <expected>
#include <format>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <print>
#include <string>
#include <tracy/Tracy.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <sys/types.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "gauge/renderer/gltf.hpp"
#include "thirdparty/imgui/imgui.h"

#include "thirdparty/imgui/backends/imgui_impl_sdl3.h"
#include "thirdparty/imgui/backends/imgui_impl_vulkan.h"

#define TRACY_VK_USE_SYMBOL_TABLE
#define CUSTUM_VALIDATION_LAYER_DEBUG_CALLBACK 1
#define USE_VULKAN_DEBUG 1

using namespace Gauge;

extern App* gApp;

glm::vec4 col{0.0f, 1.0f, 0.0f, 1.0f};

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
    VkPhysicalDeviceVulkan12Features device_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
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
    auto physical_device_ret =
        selector.set_surface(p_surface)
            .set_minimum_version(1, 3)
            .set_required_features_12(device_features_12)
            .set_required_features_13(device_features_13)
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

static std::expected<void, std::string>
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
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    VkCommandPool cmd_pool;
    VK_CHECK_RET(
        vkCreateCommandPool(ctx.device, &cmd_pool_create_info, nullptr, &cmd_pool),
        "Could not create command pool");

    return cmd_pool;
}

std::expected<VkCommandBuffer, std::string>
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
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VK_CHECK_RET(vkCreateFence(ctx.device, &fence_info, nullptr, &frame.queue_submit_fence),
                     "Could not create render fence");

        VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VK_CHECK_RET(vkCreateSemaphore(ctx.device, &semaphore_info, nullptr, &frame.swapchain_acquire_semaphore),
                     "Could not create acquire semaphore");

#ifdef TRACY_ENABLE
        frame.tracy_context = TracyVkContext(ctx.physical_device, ctx.device, ctx.graphics_queue, frame.cmd);
        std::string tacy_context_name = std::format("Frame In-Flight Index {}", i);
        TracyVkContextName(frame.tracy_context, tacy_context_name.c_str(), tacy_context_name.size());
        frames_in_flight.emplace_back(frame);
#endif

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

    VkRenderingAttachmentInfo color_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = swapchain.image_views[p_next_image_index],
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
}

std::expected<void, std::string>
RendererVulkan::CreateSwapchain(bool recreate) {
    vkb::SwapchainBuilder swapchain_builder{ctx.device};
    if (recreate) {
        swapchain_builder.set_old_swapchain(swapchain.vkb_swapchain);
        vkDeviceWaitIdle(ctx.device);
    }
    auto swapchain_ret =
        swapchain_builder
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(window_size.width, window_size.height)
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

std::expected<Pipeline, std::string>
RendererVulkan::CreateGraphicsPipeline(std::string p_name) {
    std::string ga = "triangle.spv";
    auto shader_module_result = ShaderModule::FromFile(ctx, "triangle.spv");
    CHECK_RET(shader_module_result);
    ShaderModule shader_module = shader_module_result.value();

    GraphicsPipelineBuilder builder("triangle");
    return builder
        .SetVertexStage(shader_module.handle, "VertexMain")
        .SetFragmentStage(shader_module.handle, "FragmentMain")
        .AddPushConstantRange((VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushConstants))
        .SetImageFormat(swapchain.image_format)
        .build(ctx);
}

static void SetupImGuiStyle() {
    // Fork of Everforest style from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(8.0f, 3.0f);
    style.WindowRounding = 4.599999904632568f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(4.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 13.0f;
    style.ScrollbarRounding = 16.0f;
    style.GrabMinSize = 20.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.TabBorderSize = 1.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.8745098114013672f, 0.8705882430076599f, 0.8392156958580017f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5843137502670288f, 0.572549045085907f, 0.5215686559677124f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5960784554481506f, 0.5921568870544434f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5960784554481506f, 0.5921568870544434f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 0.9725490212440491f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 0.6094420552253723f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 0.4313725531101227f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 0.9019607901573181f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

std::expected<void, std::string>
RendererVulkan::InitializeImGui() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        return std::unexpected("Could not initialize ImGui SDL3 for Vulkan");
    }

    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
    };
    VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = (uint32_t)std::size(pool_sizes),
        .pPoolSizes = pool_sizes,
    };
    VkDescriptorPool imgui_pool{};
    VK_CHECK_RET(vkCreateDescriptorPool(ctx.device, &pool_info, nullptr, &imgui_pool),
                 "Could not create ImGui descriptor pool");

    ImGui_ImplVulkan_InitInfo imgui_info{
        .Instance = ctx.instance.instance,
        .PhysicalDevice = ctx.physical_device.physical_device,
        .Device = ctx.device.device,
        .Queue = ctx.graphics_queue,
        .DescriptorPool = imgui_pool,
        .MinImageCount = 2,
        .ImageCount = 2,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain.image_format,
        },
    };
    if (!ImGui_ImplVulkan_Init(&imgui_info)) {
        return std::unexpected("Could not initialize ImGui for Vulkan");
    };

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    SetupImGuiStyle();

    return {};
}

std::expected<void, std::string>
RendererVulkan::Initialize(SDL_Window* p_sdl_window) {
    window = p_sdl_window;

    CHECK_RET(
        CreateInstance()
            .and_then([this, p_sdl_window](vkb::Instance p_instance) {
                ctx.instance = p_instance;
                return CreateSurface(p_instance, p_sdl_window);
            })
            .and_then([this](VkSurfaceKHR p_surface) {
                surface = p_surface;
                int w, h{};
                SDL_GetWindowSize(window, &w, &h);
                window_size = {.width = (uint)w, .height = (uint)h};
                return CreatePhysicalDevice(ctx.instance, surface);
            })
            .and_then([this](vkb::PhysicalDevice p_physical_device) {
                ctx.physical_device = p_physical_device;
                return CreateDevice(p_physical_device);
            })
            .and_then([this](vkb::Device p_device) {
                ctx.device = p_device;
                return InitializeVolk(ctx.instance, ctx.device);
            })
            .and_then([&]() {
                return InitializeVulkanMemoryAllocator(ctx);
            })
            .and_then([this]() {
                SetDebugName((uint64_t)ctx.instance.instance, VK_OBJECT_TYPE_INSTANCE, "Primary instance");
                SetDebugName((uint64_t)ctx.physical_device.physical_device, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "Primary physical device");
                SetDebugName((uint64_t)ctx.device.device, VK_OBJECT_TYPE_DEVICE, "Primary device");
                SetDebugName((uint64_t)surface, VK_OBJECT_TYPE_SURFACE_KHR, "Main window surface");
                return GetQueue(ctx.device);
            })
            .and_then([this](VkQueue p_queue) {
                ctx.graphics_queue = p_queue;
                SetDebugName((uint64_t)ctx.graphics_queue, VK_OBJECT_TYPE_QUEUE, "Graphics queue");
                return CreateFrameData();
            })
            .and_then([this]() {
                return CreateSwapchain();
            })
            .and_then([this]() {
                return InitializeImGui();
            }));

    auto graphics_pipeline_result = CreateGraphicsPipeline("Primary graphics pipeline");
    CHECK_RET(graphics_pipeline_result);
    graphics_pipeline = graphics_pipeline_result.value();

    render_state.viewports.emplace_back(Viewport{
        .width = static_cast<float>(window_size.width),
        .height = static_cast<float>(window_size.height),
        .fill_window = true,
    });

    auto model = glTF::FromFile("box.glb");

    initialized = true;
    return {};
}

void RendererVulkan::RenderViewport(CommandBufferVulkan* cmd, const Viewport& p_viewport, uint p_next_image_index) const {
    TracyVkZone(GetCurrentFrame().tracy_context, cmd->GetHandle(), "Viewport");

    VkRenderingAttachmentInfo rendering_attachement_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain.image_views[p_next_image_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {
            .color = {{0.0f, 0.0f, 0.0f, 0.0f}},
        }};

    VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .flags = 0,
        .renderArea =
            {
                .offset = {.x = 0, .y = 0},
                .extent =
                    {
                        .width = (uint)p_viewport.width,
                        .height = (uint)p_viewport.height,
                    },
            },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &rendering_attachement_info,
    };

    vkCmdBeginRendering(cmd->GetHandle(), &rendering_info);

    glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)(1920.0 / 1080.0), 1000.0f, 0.1f);
    projection[1][1] *= -1.0f;
    cmd->BindPipeline(graphics_pipeline);
    PushConstants push_constants{
        .model_matrix = glm::mat4(1.0f),
        .view_projection = projection,
        .vertex_buffer_address = 0,
    };
    vkCmdPushConstants(cmd->GetHandle(), graphics_pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push_constants);

    VkViewport vk_viewport{
        p_viewport.position.x, p_viewport.position.y,
        p_viewport.width, p_viewport.height,
        0.0f, 1.0f};
    VkRect2D scissor{VkOffset2D{}, swapchain.extent};
    vkCmdSetViewport(cmd->GetHandle(), 0, 1, &vk_viewport);
    vkCmdSetScissor(cmd->GetHandle(), 0, 1, &scissor);
    vkCmdDraw(cmd->GetHandle(), 3, 1, 0, 0);

    vkCmdEndRendering(cmd->GetHandle());
}

void RendererVulkan::RecordCommands(CommandBufferVulkan* cmd, uint p_next_image_index) const {
    ZoneScoped;
    TracyVkZone(GetCurrentFrame().tracy_context, cmd->GetHandle(), "Draw");

    cmd->transition_image(swapchain.images[p_next_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    for (const auto& viewport : render_state.viewports) {
        RenderViewport(cmd, viewport, p_next_image_index);
    };

    RenderImGui(cmd, p_next_image_index);

    cmd->transition_image(swapchain.images[p_next_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void RendererVulkan::Draw() {
    ZoneScoped;
    {
        ZoneScopedN("ImGui calls");
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Settings")) {
        }
        ImGui::End();
        /*
                if (ImGui::BeginMainMenuBar()) {
                    if (ImGui::BeginMenu("Project")) {
                        if (ImGui::MenuItem("New projext...")) {
                        }
                        if (ImGui::MenuItem("Quit", "CTRL+Q")) {
                            gApp->Quit();
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
                }

                ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

                ImGuiWindowClass window_class;
                window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
                ImGui::SetNextWindowClass(&window_class);
                if (ImGui::Begin("Filesystem", nullptr, ImGuiWindowFlags_NoCollapse)) {
                    float golor[] = {col.r, col.g, col.b};
                    ImGui::ColorPicker3("Triangle color", golor);
                    col.r = golor[0], col.g = golor[1], col.b = golor[2];
                }
                ImGui::End();

                ImGui::SetNextWindowClass(&window_class);
                if (!ImGui::Begin("Console")) {
                }
                ImGui::End();

                ImGui::SetNextWindowClass(&window_class);
                if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar)) {
                    if (ImGui::BeginMenuBar()) {
                        if (ImGui::BeginMenu("Viewport")) {
                            bool wireframe = false;  // TODO
                            ImGui::Checkbox("Wireframe", &wireframe);
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenuBar();
                    }
                }
                auto position = ImGui::GetWindowPos();
                auto size = ImGui::GetWindowSize();
                viewport = {.position{.x = position.x, .y = position.y}, .width = size.x, .height = size.y};
                ImGui::End();
        */
        ImGui::Render();
    }
    FrameData current_frame = GetCurrentFrame();
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

    VkCommandBuffer current_command_buffer = current_frame.cmd;
    CommandBufferVulkan cmd{current_command_buffer};

    CHECK(cmd.Begin());
    RecordCommands(&cmd, next_image_index);
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

vkb::Instance const*
RendererVulkan::GetInstance() const {
    return &ctx.instance;
}

RendererVulkan::FrameData
RendererVulkan::GetCurrentFrame() const {
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
        if (viewport.fill_window) {
            viewport.width = p_width;
            viewport.height = p_height;
        }
    }
    CHECK(CreateSwapchain(true));
}