#pragma once

#include <SDL3/SDL_events.h>
#include <gauge/common.hpp>
#include <gauge/core/string_id.hpp>

namespace YAML {
class Node;
}

namespace Gauge {

class Node;

// --- Factory macros ---
// To be included in every component's header.
#define COMPONENT_FACTORY_HEADER(class)                           \
    class() {}                                                    \
    class(YAML::Node p_data);                                     \
    static void Instantiate(YAML::Node p_data, Ref<Node> p_node); \
                                                                  \
   public:                                                        \
    static bool registered;

// To be included in every component's implementation file, followed by
// { code initializing component using YAML node stored in p_data }
#define COMPONENT_FACTORY_IMPL(class, name)                                 \
    void class ::Instantiate(YAML::Node p_data, Ref<Node> p_node) {         \
        auto component = p_node->AddComponent<class>(p_data);               \
    }                                                                       \
    bool class ::registered = Component::RegisterType(#name, &Instantiate); \
    class ::class(YAML::Node p_data)
// ---- End macro magic ---

struct Component {
    using CreateFunction = void (*)(YAML::Node, std::shared_ptr<Node>);

    StringID name;
    bool visible = true;
    bool active = true;
    Node* node;

    virtual void Initialize() {}
    virtual void Update(float delta) {}
    virtual bool HandleEvent(const SDL_Event& event) { return false; };
    virtual void Draw() {}
    virtual void Finalize() {}

    void SetNode(Node* p_node) {
        node = p_node;
    }

    static bool RegisterType(StringID p_name, CreateFunction);
    static void Create(StringID p_name, YAML::Node p_data, Ref<Node> r_node);

    Component(bool p_visible = true, bool p_active = true) : visible(p_visible), active(p_active) {}

   protected:
    static StringID::Map<Component::CreateFunction>& GetCreateFunctions();
};

template <typename C>
concept IsComponent = requires(C component) {
    std::is_base_of_v<Component, C>;
    C::StaticInitialize();
};

}  // namespace Gauge
