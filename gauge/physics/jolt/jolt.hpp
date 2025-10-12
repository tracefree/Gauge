#pragma once

#include <gauge/core/pool.hpp>
#include <gauge/physics/physics.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

namespace Gauge {
namespace Physics {

template <typename JoltType, typename GaugeType>
constexpr JoltType ToJolt(GaugeType p_from);

template <typename GaugeType, typename JoltType>
constexpr GaugeType FromJolt(JoltType p_from);

template <>
inline constexpr JPH::ObjectLayer ToJolt(Physics::Layer p_from) {
    return static_cast<JPH::ObjectLayer>(p_from);
}

template <>
inline constexpr JPH::EMotionType ToJolt(Physics::MotionType p_from) {
    return static_cast<JPH::EMotionType>(p_from);
}

template <>
inline constexpr JPH::Vec3 ToJolt(Vec3 p_from) {
    return JPH::Vec3(p_from.x, p_from.y, p_from.z);
}

template <>
inline constexpr JPH::Vec3Arg ToJolt(Vec3 p_from) {
    return JPH::Vec3Arg(p_from.x, p_from.y, p_from.z);
}

template <>
inline constexpr Vec3 FromJolt(JPH::Vec3 p_from) {
    return Vec3(p_from.GetX(), p_from.GetY(), p_from.GetZ());
}

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer STATIC(0);
static constexpr JPH::BroadPhaseLayer DYNAMIC(1);
static constexpr uint AMOUNT(2);
};  // namespace BroadPhaseLayers

class JoltBackend final : public Physics::Backend {
    static constexpr uint cMaxBodies = 65536;
    static constexpr uint cNumBodyMutexes = 0;
    static constexpr uint cMaxBodyPairs = 65536;
    static constexpr uint cMaxContactConstraints = 10240;

    Pool<JPH::Shape*> shapes;
    Pool<JPH::BodyID> body_ids;

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
       public:
        BPLayerInterfaceImpl();

        virtual uint GetNumBroadPhaseLayers() const override;

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

       private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[(uint16_t)Layer::AMOUNT];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
       public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
       public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
    };

   public:  // TODO: Make private
    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    JPH::PhysicsSystem physics_system;
    JPH::BodyInterface* body_interface;
    std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator;
    std::unique_ptr<JPH::JobSystemThreadPool> job_system;

   public:
    virtual void Initialize() final override;
    virtual void Finalize() final override;
    virtual void Update(const double timestamp) final override;
    virtual void OptimizeBroadPhase() final override;

    virtual ShapeHandle ShapeCreate() final override;
    virtual ShapeHandle ShapeCreateMesh(std::vector<Vec3> p_vertices, std::vector<uint> p_indices) final override;

    virtual BodyHandle BodyCreateAndAdd(
        ShapeHandle p_shape,
        MotionType p_motion_type = MotionType::STATIC,
        Layer p_layer = Layer::STATIC,
        Vec3 p_position = Vec3::ZERO,
        Quaternion p_rotation = glm::identity<glm::quat>(),
        float p_friction = 0.2f,
        bool p_activate = true) final override;

    JoltBackend() {}
    ~JoltBackend() {}
};

}  // namespace Physics
}  // namespace Gauge