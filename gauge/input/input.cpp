#include "input.hpp"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_scancode.h>

#include <cstddef>
#include <gauge/math/common.hpp>

#include <chrono>
#include <memory>
#include <print>
#include <vector>

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
void Input::ActionSet::AddAction(const std::string& p_name, Action<bool> p_action) {
    actions_bool[p_name] = p_action;
}

template <>
void Input::ActionSet::AddAction(const std::string& p_name, Action<Vec2> p_action) {
    actions_vec2[p_name] = p_action;
}

template <>
void Input::ActionSet::AddAction(const std::string& p_name, Action<float> p_action) {
    actions_float[p_name] = p_action;
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
        } break;
        case SDL_EVENT_MOUSE_WHEEL: {
            mouse.wheel += p_event.wheel.y;
        } break;
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
        for (auto& action : set.second.GetActions<bool>()) {
            action.second.value = false;
            for (auto& binding : action.second.bindings) {
                action.second.value |= binding->GetValue();
            }
        }
        for (auto& action : set.second.GetActions<Vec2>()) {
            action.second.value = Vec2();
            for (auto& binding : action.second.bindings) {
                action.second.value += (Vec2)binding->GetValue();
            }
        }
        for (auto& action : set.second.GetActions<float>()) {
            action.second.value = 0.0f;
            for (auto& binding : action.second.bindings) {
                action.second.value += binding->GetValue();
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
    using Modifiers = std::vector<Ref<Input::Binding<bool>>>;

    // Load gamepads
    int number_gamepads = 0;
    SDL_JoystickID* joystick_ids = SDL_GetGamepads(&number_gamepads);
    if (joystick_ids == NULL) {
        std::println("Error: Could not get gamepads. Input: {}", SDL_GetError());
    } else if (number_gamepads > 0) {
        SDL_Gamepad* gamepad = SDL_OpenGamepad(joystick_ids[0]);
        if (gamepad == NULL) {
            std::println("Error: Could not open gamepad. Input: {}", SDL_GetError());
        } else {
            singleton->gamepad = gamepad;
        }
    }

    // Common keyboard bindings
    auto right_mouse_button_binding = std::make_shared<MouseButtonBinding>(MouseButton::RIGHT);
    auto middle_mouse_button_binding = std::make_shared<MouseButtonBinding>(MouseButton::MIDDLE);
    auto mouse_wheel_binding = std::make_shared<MouseWheelBinding>();
    auto shift_binding = std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_LSHIFT);
    auto space_binding = std::make_shared<KeyBinding<bool>>(SDL_SCANCODE_SPACE);
    auto wasd_binding = std::make_shared<KeyBinding<Vec2>>(
        SDL_SCANCODE_D,
        SDL_SCANCODE_A,
        SDL_SCANCODE_W,
        SDL_SCANCODE_S);
    auto qe_binding = std::make_shared<KeyBinding<float>>(
        SDL_SCANCODE_E,
        SDL_SCANCODE_Q);

    // Common gamepad bindings
    auto left_stick_binding = std::make_shared<GamepadJoystickBinding>(
        SDL_GAMEPAD_AXIS_LEFTX,
        SDL_GAMEPAD_AXIS_LEFTY,
        Vec2(1.0f, -1.0f),
        0.2f);
    auto right_stick_binding = std::make_shared<GamepadJoystickBinding>(
        SDL_GAMEPAD_AXIS_RIGHTX,
        SDL_GAMEPAD_AXIS_RIGHTY,
        Vec2(20.0f),
        0.0f);
    auto east_button_binding = std::make_shared<GamepadButtonBinding>(
        SDL_GAMEPAD_BUTTON_EAST);
    auto south_button_binding = std::make_shared<GamepadButtonBinding>(
        SDL_GAMEPAD_BUTTON_SOUTH);

    // Freecam
    ActionSet freecam_set{};
    freecam_set.AddAction<bool>(
        "grab_mouse",
        {.bindings = {right_mouse_button_binding, middle_mouse_button_binding}});

    freecam_set.AddAction<Vec2>(
        "horizontal",
        {.bindings = {std::make_shared<CombinationBinding<Vec2>>(
             (Modifiers){right_mouse_button_binding},
             wasd_binding)}});

    freecam_set.AddAction<float>(
        "vertical",
        {.bindings = {std::make_shared<CombinationBinding<float>>(
             (Modifiers){right_mouse_button_binding},
             qe_binding)}});

    freecam_set.AddAction<bool>(
        "boost",
        {.bindings = {shift_binding}});

    Ref<MouseMotionBinding> mouse_motion_binding = std::make_shared<MouseMotionBinding>();

    freecam_set.AddAction<bool>(
        "activate_look",
        {.bindings = {right_mouse_button_binding}});

    Ref<CombinationBinding<Vec2>> look_mouse_binding = std::make_shared<CombinationBinding<Vec2>>(
        (Modifiers){right_mouse_button_binding},
        mouse_motion_binding);

    freecam_set.AddAction<Vec2>(
        "look",
        {.bindings = {look_mouse_binding}});

    freecam_set.AddAction<float>(
        "zoom",
        {.bindings = {mouse_wheel_binding}});

    Ref<CombinationBinding<Vec2>> orbit_mouse_binding = std::make_shared<CombinationBinding<Vec2>>(
        (Modifiers){middle_mouse_button_binding},
        mouse_motion_binding);

    freecam_set.AddAction<Vec2>(
        "orbit",
        {.bindings = {orbit_mouse_binding}});

    Ref<CombinationBinding<Vec2>> tangential_binding = std::make_shared<CombinationBinding<Vec2>>(
        (Modifiers){middle_mouse_button_binding, shift_binding},
        mouse_motion_binding);

    freecam_set.AddAction<Vec2>(
        "tangential",
        {.bindings = {tangential_binding}});

    singleton->active_action_sets["freecam"] = freecam_set;

    // Third person camera
    ActionSet third_person_camera;
    third_person_camera.AddAction<float>(
        "zoom",
        {.bindings = {mouse_wheel_binding}});
    third_person_camera.AddAction<Vec2>(
        "orbit",
        {.bindings = {
             mouse_motion_binding,
             right_stick_binding,
         }});
    singleton->active_action_sets["third_person_camera"] = third_person_camera;

    // Character controller
    ActionSet character;
    character.AddAction<Vec2>(
        "move",
        {.bindings = {
             wasd_binding,
             left_stick_binding,
         }});
    character.AddAction<bool>(
        "sprint",
        {.bindings = {
             shift_binding,
             east_button_binding,
         }});
    character.AddAction<bool>(
        "jump",
        {.bindings = {
             space_binding,
             south_button_binding,
         }});
    singleton->active_action_sets["character"] = character;

    // TODO: Move to static initialization of TransformGizmo
    ActionSet transform_set{};
    transform_set.AddAction<Vec2>(
        "mouse_motion",
        {.bindings = {mouse_motion_binding}});
    singleton->active_action_sets["transform"] = transform_set;
}
