#pragma once

#include <gauge/common.hpp>
#include <string>

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
        component->Initialize();                                            \
    }                                                                       \
    bool class ::registered = Component::RegisterType(#name, &Instantiate); \
    class ::class(YAML::Node p_data)
// ---- End macro magic ---

struct Component {
    using CreateFunction = void (*)(YAML::Node, std::shared_ptr<Node>);

    // static std::unordered_map<std::string, CreateFunction> create_functions;

    std::string name;
    bool active = true;
    bool visible = true;
    Node* node;

    virtual void Initialize() {}
    virtual void Update(float delta) {}
    virtual void Draw() {}
    virtual void Finalize() {}

    void SetNode(Node* p_node) {
        node = p_node;
    }

    static bool RegisterType(const std::string& p_name, CreateFunction);
    static void Create(const std::string& p_name, YAML::Node p_data, Ref<Node> r_node);

   protected:
    static std::unordered_map<std::string, Component::CreateFunction>& GetCreateFunctions();
};

template <typename C>
concept IsComponent = requires(C component) {
    std::is_base_of_v<Component, C>;
};

}  // namespace Gauge
