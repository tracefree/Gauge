#pragma once

#include <sys/types.h>
#include <gauge/math/transform.hpp>
#include <vector>

namespace Gauge {
namespace Physics {

using ShapeHandle = uint;
using BodyHandle = uint;

enum class Layer : uint16_t {
    STATIC = 0,
    DYNAMIC = 1,
    AMOUNT = 2,
};

enum class MotionType : uint8_t {
    STATIC,
    KINEMATIC,
    DYNAMIC,
};

class Backend;

template <typename T>
concept IsPhysicsBackend = requires(T backend) {
    std::is_base_of_v<Physics::Backend, T>;
};

class Backend {
    static Backend* singleton;

   public:
    template <IsPhysicsBackend PhysicsBackend>
    static void InitializeBackend() {
        if (PhysicsBackend::singleton)
            return;

        PhysicsBackend::singleton = static_cast<PhysicsBackend*>(new PhysicsBackend);
        PhysicsBackend::singleton->Initialize();
    }

    static void FinalizeBackend() {
        if (!singleton)
            return;
        singleton->Finalize();
        delete Backend::singleton;
    }

    virtual void Initialize() {};
    virtual void Finalize() {};
    virtual void Update(const double timestamp) {}
    virtual void OptimizeBroadPhase() {}

    virtual ShapeHandle ShapeCreate() { return 0; }
    virtual ShapeHandle ShapeCreateMesh(std::vector<Vec3> p_vertices, std::vector<uint> p_indices) { return 0; };

    virtual BodyHandle BodyCreateAndAdd(
        ShapeHandle p_shape,
        MotionType p_motion_type = MotionType::STATIC,
        Layer p_layer = Layer::STATIC,
        Vec3 p_position = Vec3::ZERO,
        Quaternion p_rotation = glm::identity<glm::quat>(),
        float p_friction = 0.2f,
        bool p_activate = true) { return 0; }

    static Backend* Get() {
        return singleton;
    }

    Backend() {}
    virtual ~Backend() {}
};

inline Backend* Get() {
    return Backend::Get();
}

template <IsPhysicsBackend T>
void InitializeBackend() {
    Backend::InitializeBackend<T>();
}

inline void FinalizeBackend() {
    Backend::FinalizeBackend();
}

}  // namespace Physics
}  // namespace Gauge