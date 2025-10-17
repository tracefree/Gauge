#include "character_controller.hpp"

#include <gauge/components/camera.hpp>
#include <gauge/input/input.hpp>
#include <gauge/physics/jolt/character.hpp>
#include <gauge/scene/node.hpp>
#include <gauge/scene/yaml.hpp>

using namespace Gauge;

extern Ref<Node> player;
extern Ref<Camera> camera;

void CharacterController::Initialize() {
    player = Ref<Node>(node->self.lock());
    character = std::unique_ptr<Physics::Character>(static_cast<Physics::Character*>(new Physics::JoltCharacter));
    character->Initialize();
}

void CharacterController::Update(float delta) {
    const auto movement_input = Input::Get()->GetActionValue<Vec2>("character/move");

    if (!movement_input.IsZero()) {
        const Vec3 direction = camera->GetPlanarRotation() * Vec3(movement_input.x, 0.0f, -movement_input.y);
        const float speed = Input::Get()->GetActionValue<bool>("character/sprint") ? 6.0f : 4.0f;
        velocity = Vec3(
            (speed * direction).x,
            velocity.y,
            (speed * direction).z);

        Quaternion target_rotation = glm::normalize(glm::quatLookAt(-direction, Vec3::UP));
        float weight = (1.0f - std::exp(-(float(delta) / 0.025f)));
        node->SetRotation(glm::slerp(node->GetRotation(), target_rotation, weight));
    } else if (character->IsOnGround()) {
        velocity = Vec3::ZERO;
    }

    velocity.y -= 9.81 * delta;
    if (character->IsOnGround()) {
        const auto jumping = Input::Get()->GetActionValue<bool>("character/jump");
        velocity.y = jumping ? 4.0f : 0.0f;
    }

    character->SetVelocity(velocity);
    character->Update(delta);
    node->SetPosition(character->GetPosition());

    // node->Move(velocity * delta);
}

COMPONENT_FACTORY_IMPL(CharacterController, character_controller) {
}