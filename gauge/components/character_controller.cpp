#include "character_controller.hpp"

#include <gauge/components/camera.hpp>
#include <gauge/input/input.hpp>
#include <gauge/scene/node.hpp>
#include <gauge/scene/yaml.hpp>

#include <memory>

using namespace Gauge;

extern Ref<Node> player;
extern Ref<Camera> camera;

void CharacterController::Initialize() {
    player = Ref<Node>(node->self.lock());
}

void CharacterController::Update(float delta) {
    const auto movement_input = Input::Get()->GetActionValue<Vec2>("character/move");
    const auto jumping = Input::Get()->GetActionValue<bool>("character/jump");
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
    } else if (node->GetGlobalTransform().position.y <= 0.0f) {
        velocity = Vec3(0.0, velocity.y, 0.0);
    }

    if (node->GetGlobalTransform().position.y > 0.0f) {
        velocity.y -= 9.81 * delta;
    } else {
        velocity.y = jumping ? 4.0f : 0.0f;
    }

    node->Move(velocity * delta);
}

COMPONENT_FACTORY_IMPL(CharacterController, character_controller) {
}