#pragma once

#include <SDL3/SDL_events.h>
#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/math/transform.hpp>
#include <gauge/renderer/aabb.hpp>
#include <memory>
#include <print>
#include <string>
#include <typeindex>

namespace Gauge {

class Node {
   public:
    StringID name;
    Transform local_transform;
    Transform global_transform;
    AABB aabb;

    std::vector<Ref<Node>> children;
    std::weak_ptr<Node> parent;
    std::weak_ptr<Node> self;

   protected:
    std::unordered_map<std::type_index, Ref<Component>> component_table;
    std::vector<Ref<Component>> components;
    static Pool<std::weak_ptr<Node>> pool;

   public:
    bool active = true;
    bool visible = true;
    Handle<std::weak_ptr<Node>> handle;

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

    Transform GetTransform() const;
    Transform GetGlobalTransform() const;
    void RefreshTransform();
    void RefreshTransform(Transform const& p_parent_transform);

    std::vector<Ref<Component>> const& GetComponents() const;

    bool HasParent() const;
    void AddChild(const Ref<Node>& p_node);
    bool HasChild(StringID p_name) const;
    Ref<Node> GetChild(StringID p_name) const;
    void RemoveChildren();

    void Draw() const;
    void Update(float delta);
    void ProcessInput(const SDL_Event& event);
    void Cleanup();

    template <IsComponent C>
    void AddComponent(Ref<C> p_component) {
        p_component->SetNode(this);
        components.push_back(p_component);
        component_table[std::type_index(typeid(C))] = p_component;
    }

    template <IsComponent C, typename... Args>
    Ref<C> AddComponent(Args... p_constructor_arguments) {
        auto component = std::make_shared<C>(p_constructor_arguments...);
        AddComponent(component);
        return component;
    }

    template <IsComponent C>
    Ref<C> GetComponent() {
        return std::static_pointer_cast<C>(component_table[std::type_index(typeid(C))]);
    }

    template <IsComponent C>
    bool HasComponent() {
        return component_table.contains(std::type_index(typeid(C)));
    }

    static Ref<Node> Create(const std::string& p_name = "[Node]") {
        Ref<Node> node = std::make_shared<Node>(p_name);
        node->self = node;
        node->handle = pool.Allocate(node);
        std::println("Created node {} with handle: {}", p_name, node->handle.ToUint());
        return node;
    }

    static std::weak_ptr<Node> Get(Handle<std::weak_ptr<Node>> p_handle) {
        auto pointer = pool.Get(p_handle);
        if (pointer == nullptr) {
            return std::weak_ptr<Node>();
        }
        return *pointer;
    }

    Node(const std::string& p_name = "[Node]") : name(p_name) {};
    ~Node() {
        if (handle.index > 0) {
            pool.Free(handle);
        }
    }
};

}  // namespace Gauge
