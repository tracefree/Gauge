#include "component.hpp"

using namespace Gauge;

std::unordered_map<std::string, Component::CreateFunction> Component::create_functions{};

bool Component::RegisterType(const std::string& p_name, Component::CreateFunction p_create_function) {
    /* FIXME
    Component::create_functions = {{"gna", CreateFunction{}}};
    if (Component::create_functions.contains(p_name)) {
        return false;
    }

    Component::create_functions[p_name] = p_create_function;
    */
    return true;
}