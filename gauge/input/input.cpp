#include "input.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <chrono>
#include <print>
#include "gauge/math/common.hpp"

using namespace Gauge;

Input* Input::singleton = nullptr;

template <>
std::unordered_map<std::string, Input::Action<bool>>& Input::ActionSet::GetActions() {
    return actions_bool;
}

template <>
std::unordered_map<std::string, Input::Action<Vec2>>& Input::ActionSet::GetActions() {
    return actions_vec2;
}

template <>
void Input::AddAction(const Action<bool>& p_action) {
}

void Input::HandleSDLEvent(const SDL_Event& p_event) {
    switch (p_event.type) {
        case SDL_EVENT_KEY_DOWN: {
            key_states[p_event.key.scancode].pressed = true;
            key_states[p_event.key.scancode].timestamp = std::chrono::steady_clock::now();
        } break;
        case SDL_EVENT_KEY_UP: {
            key_states[p_event.key.scancode].pressed = false;
            key_states[p_event.key.scancode].timestamp = std::chrono::steady_clock::now();
        } break;
        case SDL_EVENT_MOUSE_MOTION: {
            mouse_motion.x += p_event.motion.xrel;
            mouse_motion.y += p_event.motion.yrel;
        }
    }
}

void Input::ProcessActions() {
    for (auto& set : active_action_sets) {
        for (auto& action : set.second.actions_bool) {
            action.second.value = false;
            for (auto& binding : action.second.bindings) {
                action.second.value |= binding->GetValue();
            }
        }
        for (auto& action : set.second.actions_vec2) {
            for (auto& binding : action.second.bindings) {
                action.second.value = binding->GetValue();
            }
        }
    }
}

void Input::ResetFrame() {
    mouse_motion = Vec2();
}

void Input::Initialize() {
    std::println("Initializing input system...");
    singleton = new Input{};
    ActionSet freecam_set{};

    freecam_set.actions_vec2["horizontal"] = Action<Vec2>{
        .bindings = {std::make_shared<KeyBinding<Vec2>>(
            SDL_SCANCODE_D,
            SDL_SCANCODE_A,
            SDL_SCANCODE_W,
            SDL_SCANCODE_S)}};

    freecam_set.actions_bool["up"] = Action<bool>{.bindings = {std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_E)}};
    freecam_set.actions_bool["down"] = Action<bool>{.bindings = {std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_Q)}};
    freecam_set.actions_bool["boost"] = Action<bool>{.bindings = {std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_LSHIFT)}};

    singleton->active_action_sets["freecam"] = freecam_set;
}
