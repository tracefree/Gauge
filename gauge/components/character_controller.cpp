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
    const Vec2 movement_input = Input::Get()->GetActionValue<Vec2>("character/move");
    if (!movement_input.IsZero()) {
        Vec3 movement = camera->GetPlanarRotation() * Vec3(movement_input.x, 0.0f, -movement_input.y);
        const float speed = Input::Get()->GetActionValue<bool>("character/sprint") ? 6.0f : 4.0f;
        Quaternion target_rotation = glm::normalize(glm::quatLookAt(-movement, Vec3::UP));
        float weight = (1.0f - std::exp(-(float(delta) / 0.025f)));
        node->Move(movement * speed * delta);
        node->SetRotation(glm::slerp(node->GetRotation(), target_rotation, weight));
    }
}

COMPONENT_FACTORY_IMPL(CharacterController, character_controller) {
}