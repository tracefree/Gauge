#pragma once

#include <gauge/components/component.hpp>
#include <gauge/math/common.hpp>
#include <gauge/ui/window.hpp>

namespace Gauge {

struct Camera : public Component {
    const float MOUSE_SENSITIVITY = (1 / 500.0f);

    enum class Mode {
        FREEFLY,
        THIRD_PERSON,
    } mode = Mode::FREEFLY;

    float yaw{};
    float pitch{};

    float distance = 2.0f;
    float target_distance = distance;
    float min_distance = 0.1f;
    float max_distance = 0.0f;

    bool mouse_captured = false;
    Vec2 pre_grab_mouse_position{};

    Window* window;

    virtual void Initialize() final override;
    virtual void Update(float delta) final override;

    void GrabMouse();
    void ReleaseMouse();
    void Rotate(float p_yaw, float p_pitch);
    Quaternion GetRotation() const;
    Quaternion GetPlanarRotation() const;

    Mat4 GetTransformMatrix() const;
    Mat4 GetViewMatrix() const;

    Camera(Mode p_mode) : mode(p_mode) {}

    static void StaticInitialize();
    COMPONENT_FACTORY_HEADER(Camera);
};

}  // namespace Gauge