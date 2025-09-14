#include "input.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <chrono>
#include <memory>
#include <print>
#include <vector>
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
std::unordered_map<std::string, Input::Action<float>>& Input::ActionSet::GetActions() {
    return actions_float;
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
            mouse.motion.x += p_event.motion.xrel;
            mouse.motion.y += p_event.motion.yrel;
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            mouse.wheel += p_event.wheel.y;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            mouse_button_states[(MouseButton)p_event.button.button].pressed = true;
            mouse_button_states[(MouseButton)p_event.button.button].timestamp = std::chrono::steady_clock::now();
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            mouse_button_states[(MouseButton)p_event.button.button].pressed = false;
            mouse_button_states[(MouseButton)p_event.button.button].timestamp = std::chrono::steady_clock::now();
        } break;
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
        for (auto& action : set.second.actions_float) {
            for (auto& binding : action.second.bindings) {
                action.second.value = binding->GetValue();
            }
        }
    }
}

void Input::ResetFrame() {
    mouse.motion = Vec2();
    mouse.wheel = 0.0f;
}

void Input::ResetKeys() {
    for (auto& key_state : key_states) {
        key_state.second.pressed = false;
        key_state.second.timestamp = std::chrono::steady_clock::now();
    }
}

void Input::Initialize() {
    std::println("Initializing input system...");
    singleton = new Input{};
    ActionSet freecam_set{};

    Ref<MouseButtonBinding> right_mouse_button_binding = std::make_shared<MouseButtonBinding>(MouseButton::RIGHT);
    Ref<MouseButtonBinding> middle_mouse_button_binding = std::make_shared<MouseButtonBinding>(MouseButton::MIDDLE);
    Ref<MouseWheelBinding> mouse_wheel_binding = std::make_shared<MouseWheelBinding>();
    Ref<KeyBinding<bool>> shift_binding = std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_LSHIFT);
    Ref<KeyBinding<Vec2>> wasd_binding = std::make_shared<KeyBinding<Vec2>>(
        SDL_SCANCODE_D,
        SDL_SCANCODE_A,
        SDL_SCANCODE_W,
        SDL_SCANCODE_S);

    freecam_set.actions_bool["grab_mouse"] = Action<bool>{.bindings = {right_mouse_button_binding, middle_mouse_button_binding}};

    freecam_set.actions_vec2["horizontal"] = Action<Vec2>{
        .bindings = {std::make_shared<CombinationBinding<Vec2>>(
            (std::vector<Ref<Input::Binding<bool>>>){right_mouse_button_binding},
            wasd_binding)}};

    freecam_set.actions_bool["up"] = Action<bool>{
        .bindings = {std::make_shared<CombinationBinding<bool>>(
            (std::vector<Ref<Input::Binding<bool>>>){right_mouse_button_binding},
            std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_E))}};
    freecam_set.actions_bool["down"] = Action<bool>{
        .bindings = {std::make_shared<CombinationBinding<bool>>(
            (std::vector<Ref<Input::Binding<bool>>>){right_mouse_button_binding},
            std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_Q))}};
    freecam_set.actions_bool["boost"] = Action<bool>{.bindings = {shift_binding}};

    Ref<MouseMotionBinding> mouse_motion_binding = std::make_shared<MouseMotionBinding>();

    freecam_set.actions_bool["activate_look"] = Action<bool>{.bindings = {right_mouse_button_binding}};

    Ref<CombinationBinding<Vec2>> look_mouse_binding = std::make_shared<CombinationBinding<Vec2>>(
        (std::vector<Ref<Input::Binding<bool>>>){right_mouse_button_binding},
        mouse_motion_binding);

    freecam_set.actions_vec2["look"] = Action<Vec2>{.bindings = {look_mouse_binding}};
    freecam_set.actions_float["zoom"] = Action<float>{.bindings = {mouse_wheel_binding}};

    Ref<CombinationBinding<Vec2>> orbit_mouse_binding = std::make_shared<CombinationBinding<Vec2>>(
        (std::vector<Ref<Input::Binding<bool>>>){middle_mouse_button_binding},
        mouse_motion_binding);
    freecam_set.actions_vec2["orbit"] = Action<Vec2>{.bindings = {orbit_mouse_binding}};

    Ref<CombinationBinding<Vec2>> tangential_binding = std::make_shared<CombinationBinding<Vec2>>(
        (std::vector<Ref<Input::Binding<bool>>>){middle_mouse_button_binding, shift_binding},
        mouse_motion_binding);
    freecam_set.actions_vec2["tangential"] = Action<Vec2>{.bindings = {tangential_binding}};

    singleton->active_action_sets["freecam"] = freecam_set;
}
