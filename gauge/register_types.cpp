#include "register_types.hpp"

#include <gauge/components/aabb_gizmo.hpp>
#include <gauge/components/camera.hpp>
#include <gauge/components/character_controller.hpp>
#include <gauge/components/mesh_instance.hpp>
#include <gauge/components/model.hpp>
#include <gauge/input/input.hpp>
#include <gauge/physics/jolt/jolt.hpp>
#include "physics/physics.hpp"

using namespace Gauge;

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