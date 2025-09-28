#pragma once

#include <cmath>
#include <cstdlib>
#include <glm/common.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "glm/ext/vector_float3.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Gauge {

class Vec2 : public glm::vec2 {
   public:
    static const Vec2 ZERO;
    static const Vec2 ONE;

    Vec2(glm::vec2 vector) : glm::vec2(vector) {}

    using glm::vec2::vec2;
    using glm::vec2::operator+=;
    using glm::vec2::operator=;
    inline Vec2 operator+=(const Vec2& rhs) {
        return glm::vec2::operator+=((glm::vec2)rhs);
    }
    inline Vec2 operator-=(const Vec2& rhs) {
        return glm::vec2::operator-=((glm::vec2)rhs);
    }

    inline float Length() const {
        return glm::length((glm::vec2) * this);
    }

    inline float Manhatten() const {
        return std::abs(x) + std::abs(y);
    }

    inline bool IsZero() const {
        return x == 0.0f && y == 0.0f;
    }

    inline operator bool() const {
        return x != 0.0f || y != 0.0f;
    }
};

class Vec3 : public glm::vec3 {
    static const Vec3 ZERO;
    static const Vec3 ONE;
    static const Vec3 RIGHT;
    static const Vec3 LEFT;
    static const Vec3 UP;
    static const Vec3 DOWN;
    static const Vec3 BACK;
    static const Vec3 FORWARD;

    Vec3(glm::vec3 vec) : glm::vec3(vec) {}
    using glm::vec3::vec3;

    inline Vec3 operator+=(const Vec3& rhs) {
        return glm::vec3::operator+=((glm::vec3)rhs);
    }
    inline Vec3 operator-=(const Vec3& rhs) {
        return glm::vec3::operator-=((glm::vec3)rhs);
    }

    inline float Length() const {
        return glm::length((glm::vec3) * this);
    }

    inline float Manhatten() const {
        return std::abs(x) + std::abs(y) + std::abs(z);
    }

    inline bool IsZero() const {
        return x == 0.0f && y == 0.0f && z != 0.0f;
    }

    inline operator bool() const {
        return x != 0.0f || y != 0.0f || z != 0.0f;
    }
};

using Vec4 = glm::vec4;

using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

using Quaternion = glm::quat;

namespace Math {

#define HALF_PI 1.57079632679489661923f

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
