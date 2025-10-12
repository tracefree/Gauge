#include "static_body.hpp"

#include <gauge/physics/physics.hpp>
#include <gauge/scene/node.hpp>

using namespace Gauge;

void StaticBody::Initialize() {
    body = Physics::Get()->BodyCreateAndAdd(
        shape,
        Physics::MotionType::STATIC,
        Physics::Layer::STATIC,
        node->GetGlobalTransform().position,
        node->GetGlobalTransform().rotation);
}

void StaticBody::Update(float delta) {
}