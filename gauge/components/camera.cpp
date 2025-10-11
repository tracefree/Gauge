#include "camera.hpp"

#include <format>
#include <gauge/core/app.hpp>
#include <gauge/input/input.hpp>
#include <gauge/scene/node.hpp>
#include <gauge/scene/yaml.hpp>
#include <print>

#include <SDL3/SDL_mouse.h>

using namespace Gauge;

extern App* gApp;
extern Window* gWindow;
extern Ref<Node> player;

void Camera::Initialize() {
    window = gWindow;
}

void Camera::Update(float delta) {
    // Zoom
    std::string action_set;
    switch (mode) {
        case Mode::FREEFLY: {
            action_set = "freecam";
        } break;
        case Mode::THIRD_PERSON: {
            action_set = "third_person_camera";
        } break;
    };

    const float zoom = 0.05f * Input::Get()->GetActionValue<float>(std::format("{}/zoom", action_set));
    if (zoom) {
        target_distance -= target_distance * zoom;
        if (max_distance > min_distance) {
            target_distance = std::clamp(target_distance, min_distance, max_distance);
        } else {
            target_distance = std::max(target_distance, min_distance);
        }
    }
    distance = Math::Interpolate(distance, target_distance, 0.1f, delta);

    if (mode == Mode::FREEFLY) {
        // 1st person movement
        const Vec2 horizontal_movement = Input::Get()->GetActionValue<Vec2>("freecam/horizontal");
        const float vertical_movement = Input::Get()->GetActionValue<float>("freecam/vertical");
        if (horizontal_movement || vertical_movement) {
            Vec3 movement = GetRotation() *
                            Vec3(horizontal_movement.x,
                                 0.0,
                                 -horizontal_movement.y);
            movement.y += vertical_movement;
            const float speed = delta * 2.0f * (Input::Get()->GetActionValue<bool>("freecam/boost") ? 4.0f : 1.5f);
            node->Move(speed * movement);
        }

        // 1st person rotation
        const Vec2 look_mouse_motion = MOUSE_SENSITIVITY * Input::Get()->GetActionValue<Vec2>("freecam/look");
        if (look_mouse_motion) {
            Vec3 current_world_position = Vec3(GetTransformMatrix()[3]);
            Rotate(look_mouse_motion.x, -look_mouse_motion.y);
            Vec3 new_world_position = GetRotation() * Vec3::FORWARD * distance + current_world_position;
            node->SetPosition(new_world_position);
        }

        // Tangential movement
        const Vec2 tangential_movement = MOUSE_SENSITIVITY * Input::Get()->GetActionValue<Vec2>("freecam/tangential");
        if (tangential_movement) {
            const float speed = 1.0f;
            Vec3 movement =
                GetRotation() *
                Vec3(
                    -tangential_movement.x,
                    tangential_movement.y,
                    0.0f);
            node->Move(std::sqrt(distance) * speed * movement);
        } else {
            // Orbit rotation
            const Vec2 orbit_mouse_motion = MOUSE_SENSITIVITY * Input::Get()->GetActionValue<Vec2>("freecam/orbit");
            if (orbit_mouse_motion) {
                Rotate(orbit_mouse_motion.x, -orbit_mouse_motion.y);
            }
        }

        bool capture_action_active = Input::Get()->GetActionValue<bool>("freecam/grab_mouse");
        if (!mouse_captured && capture_action_active) {
            GrabMouse();
        } else if (mouse_captured && !capture_action_active) {
            ReleaseMouse();
        }

        if (mouse_captured) {
            SDL_WarpMouseInWindow(window->GetSDLHandle(), pre_grab_mouse_position.x, pre_grab_mouse_position.y);
        }
    }

    else if (mode == Mode::THIRD_PERSON) {
        // Orbit rotation
        const Vec2 orbit_mouse_motion = MOUSE_SENSITIVITY * Input::Get()->GetActionValue<Vec2>("third_person_camera/orbit");
        if (orbit_mouse_motion) {
            Rotate(orbit_mouse_motion.x, -orbit_mouse_motion.y);
        }

        if (player) {
            const Vec3 new_position = Math::Interpolate(node->GetPosition(), (player->GetGlobalTransform().position + Vec3::UP * 1.6f), 0.1f, delta);
            node->SetPosition(new_position);
        }
    }

    gApp->renderer->ViewportSetCameraView(0, GetViewMatrix());
}

void Camera::GrabMouse() {
    mouse_captured = true;
    SDL_GetMouseState(&pre_grab_mouse_position.x, &pre_grab_mouse_position.y);
    SDL_HideCursor();
}

void Camera::ReleaseMouse() {
    mouse_captured = false;
    SDL_ShowCursor();
}

void Camera::Rotate(float p_yaw, float p_pitch) {
    yaw += p_yaw;
    pitch = std::clamp(pitch + p_pitch, -HALF_PI, HALF_PI);
}

Quaternion Camera::GetRotation() const {
    return glm::angleAxis(yaw, Vec3::DOWN) *
           glm::angleAxis(pitch, Vec3::RIGHT);
}

Quaternion Camera::GetPlanarRotation() const {
    return glm::angleAxis(-yaw, Vec3(0.0f, 1.0f, 0.0f));
}

Mat4 Camera::GetTransformMatrix() const {
    return glm::translate(Mat4(1.0f), node->global_transform.position) *
           glm::toMat4(glm::angleAxis(yaw, Vec3::DOWN) *
                       glm::angleAxis(pitch, Vec3::RIGHT)) *
           glm::translate(Mat4(1.0f), Vec3::BACK * distance);
}

Mat4 Camera::GetViewMatrix() const {
    return glm::inverse(GetTransformMatrix());
}

void Camera::StaticInitialize() {
    std::println("camera init");
}

COMPONENT_FACTORY_IMPL(Camera, camera) {
}