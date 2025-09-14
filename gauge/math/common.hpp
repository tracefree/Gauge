#pragma once

#include <glm/common.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Gauge {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

using Quaternion = glm::quat;

namespace Math {

float Wrap(float x, float max);
float Wrap(float x, float min, float max);

double Wrap(double x, double max);
double Wrap(double x, double min, double max);

template <typename T>
T Interpolate(T a, T b, float duration, float delta) {
    return a + (b - a) * (1.0f - std::exp(-delta / duration));
}

}  // namespace Math

}  // namespace Gauge
