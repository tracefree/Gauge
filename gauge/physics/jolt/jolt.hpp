#pragma once

#include <gauge/physics/physics.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Core.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <memory>

namespace Gauge {
namespace Physics {

namespace Layers {
static constexpr JPH::ObjectLayer STATIC = 0;
static constexpr JPH::ObjectLayer DYNAMIC = 1;
static constexpr JPH::ObjectLayer AMOUNT = 2;
};  // namespace Layers

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer STATIC(Layers::STATIC);
static constexpr JPH::BroadPhaseLayer DYNAMIC(Layers::DYNAMIC);
static constexpr uint AMOUNT(Layers::AMOUNT);
};  // namespace BroadPhaseLayers

class JoltBackend : public Physics::Backend {
    static constexpr uint cMaxBodies = 65536;
    static constexpr uint cNumBodyMutexes = 0;
    static constexpr uint cMaxBodyPairs = 65536;
    static constexpr uint cMaxContactConstraints = 10240;

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
       public:
        BPLayerInterfaceImpl();

        virtual uint GetNumBroadPhaseLayers() const override;

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

       private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::AMOUNT];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
       public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
       public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
    };

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
};

}  // namespace Physics
}  // namespace Gauge