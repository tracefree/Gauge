#pragma once

#include <gauge/common.hpp>
#include <gauge/math/common.hpp>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace Gauge {

enum class MouseButton {
    LEFT = SDL_BUTTON_LEFT,
    RIGHT = SDL_BUTTON_RIGHT,
    MIDDLE = SDL_BUTTON_MIDDLE,
};

class Input {
   protected:
    static Input* singleton;

   public:
    template <typename T>
    class Binding {
       public:
        virtual T GetValue() = 0;
    };

    template <typename T>
    class Action {
       public:
        std::string name;
        std::vector<Ref<Binding<T>>> bindings;
        T value;

       public:
        T GetValue() { return value; }
    };

    class ActionSet {
       protected:
        std::unordered_map<std::string, Action<bool>> actions_bool;
        std::unordered_map<std::string, Action<Vec2>> actions_vec2;
        std::unordered_map<std::string, Action<float>> actions_float;

       public:
        template <typename T>
        std::unordered_map<std::string, Action<T>>& GetActions();

        template <typename T>
        Action<T> GetAction(const std::string& p_name) {
            return GetActions<T>()[p_name];
        }

        template <typename T>
        void AddAction(const std::string& p_name, Action<T> p_action);
    };

    struct Mouse {
        Vec2 motion{};
        float wheel{};
    } mouse;

    struct KeyState {
        bool pressed{};
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<SDL_Scancode, KeyState> key_states;
    std::unordered_map<MouseButton, KeyState> mouse_button_states;
    std::unordered_map<std::string, ActionSet> active_action_sets;

   public:
    void HandleSDLEvent(const SDL_Event& p_event);
    void ProcessActions();
    void ResetFrame();
    void ResetKeys();

    template <typename T>
    void AddAction(const Action<T>& p_action);

    void AddActionSet(const ActionSet& p_action_set);

    inline KeyState GetKeyState(SDL_Scancode p_scancode) { return key_states[p_scancode]; }
    inline KeyState GetMouseButtonState(MouseButton p_mouse_button) { return mouse_button_states[p_mouse_button]; }
    inline bool IsMousePressed(MouseButton p_mouse_button) { return mouse_button_states[p_mouse_button].pressed; }

    template <typename T>
    T GetActionValue(const std::string& p_action_id) {
        uint delimiter_position = p_action_id.find("/");
        const auto set_name = p_action_id.substr(0, delimiter_position);
        const auto action_name = p_action_id.substr(delimiter_position + 1, p_action_id.length());

        return active_action_sets[set_name].GetAction<T>(action_name).GetValue();
    }

    static void Initialize();

    static void Finalize() {
        delete singleton;
    }

    static Input* Get() {
        return singleton;
    }
};

// --- KeyBinding ---

template <typename T>
class KeyBinding;

template <>
class KeyBinding<bool> : public Input::Binding<bool> {
   public:
    SDL_Scancode scancode{};

    bool GetValue() final override {
        return Input::Get()->GetKeyState(scancode).pressed;
    }

    KeyBinding(SDL_Scancode p_scancode) : scancode(p_scancode) {}
};

template <>
class KeyBinding<float> : public Input::Binding<float> {
   public:
    struct {
        SDL_Scancode positive{};
        SDL_Scancode negative{};
    } scancodes;

    float GetValue() final override {
        float p = (float)Input::Get()->GetKeyState(scancodes.positive).pressed;
        float n = (float)Input::Get()->GetKeyState(scancodes.negative).pressed;
        return p - n;
    }

    KeyBinding(SDL_Scancode p_scancode_positive,
               SDL_Scancode p_scancode_negative)
        : scancodes(p_scancode_positive,
                    p_scancode_negative) {}
};

template <>
class KeyBinding<Vec2> : public Input::Binding<Vec2> {
   public:
    struct {
        SDL_Scancode positive_x{};
        SDL_Scancode negative_x{};
        SDL_Scancode positive_y{};
        SDL_Scancode negative_y{};
    } scancodes;

    Vec2 GetValue() final override {
        float px = (float)Input::Get()->GetKeyState(scancodes.positive_x).pressed;
        float nx = (float)Input::Get()->GetKeyState(scancodes.negative_x).pressed;
        float py = (float)Input::Get()->GetKeyState(scancodes.positive_y).pressed;
        float ny = (float)Input::Get()->GetKeyState(scancodes.negative_y).pressed;
        if ((px - nx == 0) && (py - ny == 0)) {
            return Vec2();
        }
        return glm::normalize(
            Vec2(px - nx, py - ny));
    }

    KeyBinding() {}
    KeyBinding(SDL_Scancode p_scancode_positive_x,
               SDL_Scancode p_scancode_negative_x,
               SDL_Scancode p_scancode_positive_y,
               SDL_Scancode p_scancode_negative_y)
        : scancodes(p_scancode_positive_x,
                    p_scancode_negative_x,
                    p_scancode_positive_y,
                    p_scancode_negative_y) {}
};

// --- MouseButtonBinding ---

class MouseButtonBinding : public Input::Binding<bool> {
   public:
    MouseButton mouse_button;

    bool GetValue() final override {
        return Input::Get()->GetMouseButtonState(mouse_button).pressed;
    }

    MouseButtonBinding() {}
    MouseButtonBinding(MouseButton p_mouse_button) : mouse_button(p_mouse_button) {}
};

// --- MouseMotionBinding ---

class MouseMotionBinding : public Input::Binding<Vec2> {
   public:
    Vec2 GetValue() final override {
        return Input::Get()->mouse.motion;
    }
};

// --- MouseWheelBinding
class MouseWheelBinding : public Input::Binding<float> {
   public:
    float GetValue() final override {
        return Input::Get()->mouse.wheel;
    }
};

// --- CombinationBinding ---

template <typename T>
class CombinationBinding : public Input::Binding<T> {
   public:
    std::vector<Ref<Input::Binding<bool>>> activation_bindings;
    Ref<Input::Binding<T>> target_binding;
    bool require_all_activation_bindings = true;
    T default_value = T();

    T GetValue() final override {
        bool activate = require_all_activation_bindings;
        for (auto& activation_binding : activation_bindings) {
            if (require_all_activation_bindings) {
                activate = activate && activation_binding->GetValue();
            } else {
                activate = activation_binding->GetValue();
                if (activate) {
                    break;
                }
            }
        }
        if (activate) {
            return target_binding->GetValue();
        } else {
            return default_value;
        }
    }

    CombinationBinding() {}
    CombinationBinding(
        std::vector<Ref<Input::Binding<bool>>> p_activation_bindings,
        Ref<Input::Binding<T>> p_target_binding)
        : activation_bindings(p_activation_bindings),
          target_binding(p_target_binding) {}
};

}  // namespace Gauge