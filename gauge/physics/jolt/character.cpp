#include "character.hpp"

#include <gauge/physics/jolt/jolt.hpp>
#include <memory>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

using namespace Gauge;

void Physics::JoltCharacter::Initialize() {
    auto jolt = static_cast<JoltBackend*>(Physics::Get());

    auto shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0.0, 0.9, 0.0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.6f, 0.3f)).Create().Get();
    const auto settings = std::make_unique<JPH::CharacterVirtualSettings>();
    settings->mMaxSlopeAngle = 0.0;
    settings->mShape = shape;
    settings->mInnerBodyShape = shape;
    settings->mInnerBodyLayer = ToJolt<JPH::ObjectLayer>(Physics::Layer::DYNAMIC);
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.3f);
    jolt_character = std::make_unique<JPH::CharacterVirtual>(settings.get(), JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, &(jolt->physics_system));
}

void Physics::JoltCharacter::Finalize() {
}

void Physics::JoltCharacter::SetVelocity(Vec3 p_velocity) {
    jolt_character->SetLinearVelocity(Physics::ToJolt<JPH::RVec3>(p_velocity));
}

Vec3 Physics::JoltCharacter::GetPosition() const {
    return FromJolt<Vec3>(jolt_character->GetPosition());
}

void Physics::JoltCharacter::Update(float delta) {
    auto jolt = static_cast<JoltBackend*>(Physics::Get());
    const JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    jolt_character->ExtendedUpdate(
        float(delta),
        JPH::Vec3(0.0, -9.81f, 0.0),
        update_settings,
        jolt->physics_system.GetDefaultBroadPhaseLayerFilter(ToJolt<JPH::ObjectLayer>(Layer::DYNAMIC)),
        jolt->physics_system.GetDefaultLayerFilter(ToJolt<JPH::ObjectLayer>(Layer::DYNAMIC)),
        {},
        {},
        *jolt->temp_allocator);
}

bool Physics::JoltCharacter::IsOnGround() const {
    return jolt_character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
}