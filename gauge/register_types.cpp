#include "register_types.hpp"

#include <gauge/components/aabb_gizmo.hpp>
#include <gauge/components/camera.hpp>
#include <gauge/components/character_controller.hpp>
#include <gauge/components/light/point_light.hpp>
#include <gauge/components/mesh_instance.hpp>
#include <gauge/components/model.hpp>
#include <gauge/components/physics/static_body.hpp>
#include <gauge/core/app.hpp>
#include <gauge/input/input.hpp>
#include <gauge/physics/jolt/jolt.hpp>
#include <gauge/physics/physics.hpp>
#include <gauge/renderer/shaders/debug_line/debug_line_shader.hpp>
#include <gauge/renderer/shaders/gizmo/gizmo_shader.hpp>
#include <gauge/renderer/shaders/pbr/pbr_shader.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>

using namespace Gauge;

extern App* gApp;

template <IsComponent C>
static void RegisterComponent() {
    C component;  // TODO: Improve component registration
    C::StaticInitialize();
}

static void RegisterComponents() {
    RegisterComponent<AABBGizmo>();
    RegisterComponent<Camera>();
    RegisterComponent<CharacterController>();
    RegisterComponent<MeshInstance>();
    RegisterComponent<ModelComponent>();
    RegisterComponent<StaticBody>();
    RegisterComponent<PointLight>();
}

void Gauge::RegisterShaders() {
    auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
    renderer->RegisterShader<DebugLineShader>();
    renderer->RegisterShader<GizmoShader>();
    renderer->RegisterShader<PBRShader>();
}

void Gauge::RegisterMaterialTypes() {
    auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
    renderer->RegisterMaterialType<GPU_PBRMaterial>();
    renderer->RegisterMaterialType<GPU_BasicMaterial>();
}

void Gauge::RegisterTypes() {
    RegisterComponents();
}

void Gauge::InitializeSystems() {
    Input::Initialize();
    Physics::InitializeBackend<Physics::JoltBackend>();
}

void Gauge::FinalizeSystems() {
    Physics::FinalizeBackend();
    Input::Finalize();
}