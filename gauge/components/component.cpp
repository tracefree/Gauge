#include "component.hpp"

#include <gauge/scene/yaml.hpp>

#include <print>

using namespace Gauge;

// Avoid static initialization order problem with the Construct On First Use Idiom
// https://isocpp.org/wiki/faq/ctors#static-init-order
StringID::Map<Component::CreateFunction>& Component::GetCreateFunctions() {
    static StringID::Map<Component::CreateFunction> create_functions;
    return create_functions;
}

bool Component::RegisterType(StringID p_name, Component::CreateFunction p_create_function) {
    std::println("Registering component: {}", p_name);
    if (Component::GetCreateFunctions().contains(p_name)) {
        return false;
    }

    Component::GetCreateFunctions()[p_name] = p_create_function;
    return true;
}

void Component::Create(StringID p_name, YAML::Node p_data, Ref<Node> r_node) {
    if (!Component::GetCreateFunctions().contains(p_name)) {
        std::println("Can not load component {}", p_name);
        return;
    }
    Component::GetCreateFunctions()[p_name](p_data, r_node);
}