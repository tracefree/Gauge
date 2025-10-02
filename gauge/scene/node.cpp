#include "node.hpp"

#include <gauge/components/component.hpp>

using namespace Gauge;

Pool<std::weak_ptr<Node>> Node::pool;

Vec3 Node::GetPosition() const {
    return local_transform.position;
}

void Node::SetPosition(Vec3 p_position) {
    local_transform.position = p_position;
    RefreshTransform();
}

void Node::SetPosition(float x, float y, float z) {
    local_transform.position = Vec3(x, y, z);
    RefreshTransform();
}

void Node::Move(Vec3 p_offset) {
    local_transform.position += p_offset;
    RefreshTransform();
}

void Node::Move(float x, float y, float z) {
    local_transform.position += Vec3(x, y, z);
    RefreshTransform();
}

float Node::GetScale() const {
    return local_transform.scale;
}

void Node::SetScale(float p_scale) {
    local_transform.scale = p_scale;
    RefreshTransform();
}

void Node::ScaleBy(float p_scale) {
    local_transform.scale *= p_scale;
    RefreshTransform();
}

Quaternion Node::GetRotation() const {
    return local_transform.rotation;
}

void Node::SetRotation(Quaternion p_rotation) {
    local_transform.rotation = p_rotation;
    RefreshTransform();
}

void Node::Rotate(Vec3 p_axis, float p_angle) {
    local_transform.rotation *= Quaternion(p_axis * p_angle);
    RefreshTransform();
}

Transform Node::GetTransform() const {
    return local_transform;
}

Transform Node::GetGlobalTransform() const {
    return global_transform;
}

std::vector<Ref<Component>> const& Node::GetComponents() const {
    return components;
}

bool Node::HasParent() const {
    return parent.lock() != nullptr;
}

void Node::AddChild(const Ref<Node>& p_node) {
    assert(!p_node->HasParent());
    children.push_back(p_node);
    p_node->parent = self;
}

void Node::RemoveChildren() {
    children.clear();
}

void Node::Update(float delta) {
    if (!active)
        return;
    for (auto& component : components) {
        component->Update(delta);
    }
    for (auto& child : children) {
        child->Update(delta);
    }
}

void Node::Draw() const {
    if (!visible)
        return;
    for (const auto& component : components) {
        if (component->visible) {
            component->Draw();
        }
    }
    for (const auto& child : children) {
        child->Draw();
    }
}

void Node::RefreshTransform() {
    if (HasParent()) {
        RefreshTransform(parent.lock()->GetGlobalTransform());
    } else {
        RefreshTransform(Transform::IDENTITY);
    }
}

void Node::RefreshTransform(Transform const& p_parent_transform) {
    global_transform = p_parent_transform * local_transform;
    for (auto child : children) {
        child->RefreshTransform(global_transform);
    }
}