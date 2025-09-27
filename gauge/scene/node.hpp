#pragma once

#include <SDL3/SDL_events.h>
#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/math/transform.hpp>
#include <memory>

namespace Gauge {

class Node {
   public:
    std::string name;
    Transform local_transform;
    Transform global_transform;

    std::vector<Ref<Node>> children;
    std::weak_ptr<Node> parent;
    std::weak_ptr<Node> self;

   protected:
    std::unordered_map<const std::type_info*, Ref<Component>> component_table;
    std::vector<Ref<Component>> components;

   public:
    bool active = true;
    bool visible = true;

   public:
    Vec3 GetPosition() const;
    void SetPosition(Vec3 p_position);
    void SetPosition(float x, float y, float z);

    void Move(Vec3 p_offset);
    void Move(float x, float y, float z);

    float GetScale() const;
    void SetScale(float p_scale);
    void ScaleBy(float p_scale);

    Quaternion GetRotation() const;
    void SetRotation(Quaternion p_rotation);
    void Rotate(Vec3 p_axis, float p_angle);

    Transform GetGlobalTransform() const;
    void RefreshTransform();
    void RefreshTransform(Transform const& p_parent_transform);

    std::vector<Ref<Component>> const& GetComponents() const;

    void AddChild(const Ref<Node>& p_node);
    void Draw() const;
    void Update(float delta);
    void ProcessInput(const SDL_Event& event);
    void Cleanup();

    template <IsComponent C>
    void AddComponent(Ref<C> p_component) {
        p_component->SetNode(this);
        components.push_back(p_component);
        component_table[&typeid(C)] = p_component;
    }

    template <IsComponent C, typename... Args>
    Ref<C> AddComponent(Args... p_constructor_arguments) {
        auto component = std::make_shared<C>(p_constructor_arguments...);
        AddComponent(component);
        return component;
    }

    template <IsComponent C>
    Ref<C> GetComponent() {
        return std::static_pointer_cast<C>(component_table[&typeid(C)]);
    }

    static Ref<Node> Create(const std::string& p_name = "[Node]") {
        Ref<Node> node = std::make_shared<Node>(p_name);
        node->self = node;
        return node;
    }

    Node() {}
    Node(std::string p_name) : name(p_name) {};
};

}  // namespace Gauge
