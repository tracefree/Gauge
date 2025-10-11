#pragma once

namespace Gauge {
namespace Physics {

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