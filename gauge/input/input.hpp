#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include "gauge/common.hpp"
#include "gauge/math/common.hpp"

namespace Gauge {

struct Input {
    static Input* singleton;

    template <typename T>
    struct Binding {
        virtual T GetValue() = 0;
    };

    template <typename T>
    struct Action {
        std::string name;

        std::vector<Ref<Binding<T>>> bindings;
        T value;

        T GetValue() { return value; }
    };

    struct ActionSet {
        std::unordered_map<std::string, Action<bool>> actions_bool;
        std::unordered_map<std::string, Action<Vec2>> actions_vec2;

        template <typename T>
        std::unordered_map<std::string, Action<T>>& GetActions();
    };

    Vec2 mouse_motion{};

    struct KeyState {
        bool pressed{};
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<SDL_Scancode, KeyState> key_states;
    std::unordered_map<std::string, ActionSet> active_action_sets;

   public:
    void HandleSDLEvent(const SDL_Event& p_event);
    void ProcessActions();
    void ResetFrame();

    template <typename T>
    void AddAction(const Action<T>& p_action);

    void AddActionSet(const ActionSet& p_action_set);

    KeyState GetKeyState(SDL_Scancode p_scancode) { return key_states[p_scancode]; }

    template <typename T>
    T GetActionValue(const std::string& p_action_id) {
        uint delimiter_position = p_action_id.find("/");
        const auto set_name = p_action_id.substr(0, delimiter_position);
        const auto action_name = p_action_id.substr(delimiter_position + 1, p_action_id.length());

        return active_action_sets[set_name].GetActions<T>()[action_name].GetValue();
    }

    static void Initialize();

    static void Finalize() {
        delete singleton;
    }

    static Input* Get() {
        return singleton;
    }
};

template <typename T>
struct KeyBinding;

template <>
struct KeyBinding<bool> : public Input::Binding<bool> {
    SDL_Scancode scancode{};

    bool GetValue() final override {
        return Input::Get()->GetKeyState(scancode).pressed;
    }

    KeyBinding(SDL_Scancode p_scancode) : scancode(p_scancode) {}
};

template <>
struct KeyBinding<Vec2> : public Input::Binding<Vec2> {
    SDL_Scancode scancode_positive_x{};
    SDL_Scancode scancode_negative_x{};
    SDL_Scancode scancode_positive_y{};
    SDL_Scancode scancode_negative_y{};

    Vec2 GetValue() final override {
        bool px = Input::Get()->GetKeyState(scancode_positive_x).pressed;
        bool nx = Input::Get()->GetKeyState(scancode_negative_x).pressed;
        bool py = Input::Get()->GetKeyState(scancode_positive_y).pressed;
        bool ny = Input::Get()->GetKeyState(scancode_negative_y).pressed;
        if (((uint)px - (uint)nx == 0) && ((uint)py - (uint)ny == 0)) {
            return Vec2();
        }
        return glm::normalize(
            Vec2(
                (px ? 1.0f : 0.0f) + (nx ? -1.0f : 0.0f),
                (py ? 1.0f : 0.0f) + (ny ? -1.0f : 0.0f)));
    }

    KeyBinding(SDL_Scancode p_scancode_positive_x,
               SDL_Scancode p_scancode_negative_x,
               SDL_Scancode p_scancode_positive_y,
               SDL_Scancode p_scancode_negative_y)
        : scancode_positive_x(p_scancode_positive_x),
          scancode_negative_x(p_scancode_negative_x),
          scancode_positive_y(p_scancode_positive_y),
          scancode_negative_y(p_scancode_negative_y) {}
};

}  // namespace Gauge