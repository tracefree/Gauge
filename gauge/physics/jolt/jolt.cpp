#include "jolt.hpp"

#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "thirdparty/tracy/public/tracy/Tracy.hpp"

using namespace Gauge;
using namespace Physics;

// --- BPLayerInterfaceImpl ---
JoltBackend::BPLayerInterfaceImpl::BPLayerInterfaceImpl() {
    // Create a mapping table from object to broad phase layer
    mObjectToBroadPhase[(uint16_t)Layer::STATIC] = BroadPhaseLayers::STATIC;
    mObjectToBroadPhase[(uint16_t)Layer::DYNAMIC] = BroadPhaseLayers::DYNAMIC;
}

uint JoltBackend::BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
    return BroadPhaseLayers::AMOUNT;
}

JPH::BroadPhaseLayer JoltBackend::BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const {
    JPH_ASSERT(inLayer < ToJolt<JPH::ObjectLayer>(Layer::AMOUNT));
    return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* JoltBackend::BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const {
    switch ((JPH::BroadPhaseLayer::Type)inLayer) {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::STATIC:
            return "STATIC";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::DYNAMIC:
            return "DYNAMIC";
        default:
            JPH_ASSERT(false);
            return "INVALID";
    }
}
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

// --- ObjectVsBroadPhaseLayerFilterImpl ---
bool JoltBackend::ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
    switch (inLayer1) {
        case ToJolt<JPH::ObjectLayer>(Layer::STATIC):
            return inLayer2 == BroadPhaseLayers::DYNAMIC;
        case ToJolt<JPH::ObjectLayer>(Layer::DYNAMIC):
            return true;
        default:
            JPH_ASSERT(false);
            return false;
    }
}

// --- ObjectLayerPairFilterImpl ---
bool JoltBackend::ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const {
    switch (inObject1) {
        case ToJolt<JPH::ObjectLayer>(Layer::STATIC):
            return inObject2 == ToJolt<JPH::ObjectLayer>(Layer::DYNAMIC);  // Non moving only collides with moving
        case ToJolt<JPH::ObjectLayer>(Layer::DYNAMIC):
            return true;  // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
    }
}

class MyContactListener : public JPH::ContactListener {
   public:
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override {
        // std::cout << "Contact validate callback" << std::endl;
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
        // std::cout << "A contact was added" << std::endl;
    }

    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
        // std::cout << "A contact was persisted" << std::endl;
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
        // std::cout << "A contact was removed" << std::endl;
    }
};

class MyBodyActivationListener : public JPH::BodyActivationListener {
   public:
    virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override {
        // std::cout << "A body got activated" << std::endl;
    }

    virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override {
        // std::cout << "A body went to sleep" << std::endl;
    }
};

static MyBodyActivationListener body_activation_listener;
static MyContactListener contact_listener;

void JoltBackend::Initialize() {
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    job_system = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    physics_system.Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        broad_phase_layer_interface,
        object_vs_broadphase_layer_filter,
        object_vs_object_layer_filter);
    physics_system.SetBodyActivationListener(&body_activation_listener);
    physics_system.SetContactListener(&contact_listener);

    body_interface = &physics_system.GetBodyInterface();

    shapes.Allocate(JPH::EmptyShapeSettings().Create().Get());
}

void JoltBackend::Finalize() {
    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;
}

void JoltBackend::Update(const double timestep) {
    ZoneScoped;
    physics_system.Update(timestep, 1, temp_allocator.get(), job_system.get());
}

void JoltBackend::OptimizeBroadPhase() {
    physics_system.OptimizeBroadPhase();
}

ShapeHandle
JoltBackend::ShapeCreate() {
    auto shape = new JPH::PlaneShape(JPH::Plane::sFromPointAndNormal(JPH::Vec3(0.0, 0.0, 0.0), JPH::Vec3(0.0, 1.0, 0.0)), nullptr, 5.0f);
    return shapes.Allocate(static_cast<JPH::Shape*>(shape)).ToUint();
}

ShapeHandle
JoltBackend::ShapeCreateMesh(std::vector<Vec3> p_vertices, std::vector<uint> p_indices) {
    JPH::TriangleList triangles;
    triangles.reserve(p_indices.size() * 3);
    for (uint triangle_id = 0; triangle_id < p_indices.size(); triangle_id += 3) {
        triangles.emplace_back(JPH::Triangle(
            ToJolt<JPH::Vec3>(p_vertices[p_indices[triangle_id]]),
            ToJolt<JPH::Vec3>(p_vertices[p_indices[triangle_id + 1]]),
            ToJolt<JPH::Vec3>(p_vertices[p_indices[triangle_id + 2]])));
    }
    auto mesh_settings = new JPH::MeshShapeSettings(
        triangles,
        JPH::PhysicsMaterialList({JPH ::PhysicsMaterial::sDefault}));
    return shapes.Allocate(mesh_settings->Create().Get());
}

BodyHandle
JoltBackend::BodyCreateAndAdd(
    ShapeHandle p_shape,
    MotionType p_motion_type,
    Layer p_layer,
    Vec3 p_position,
    Quaternion p_rotation,
    float p_friction,
    bool p_activate) {
    const JPH::Shape* shape = *shapes.Get(p_shape);
    const JPH::BodyCreationSettings body_settings(
        shape,
        ToJolt<JPH::Vec3>(p_position),
        JPH::Quat::sIdentity(),
        ToJolt<JPH::EMotionType>(p_motion_type),
        ToJolt<JPH::ObjectLayer>(p_layer));
    const JPH::BodyID body_id = body_interface->CreateAndAddBody(
        body_settings,
        p_activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    body_interface->SetFriction(body_id, p_friction);
    return body_ids.Allocate(body_id).ToUint();
}
