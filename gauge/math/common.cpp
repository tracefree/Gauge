#include "common.hpp"

using namespace Gauge;

const Vec2 Vec2::ZERO = Vec2(0.0f);
const Vec2 Vec2::ONE = Vec2(1.0f);

const Vec3 Vec3::ONE = Vec3(1.0f);
const Vec3 Vec3::ZERO = Vec3(0.0f);
const Vec3 Vec3::RIGHT = Vec3(1.0f, 0.0, 0.0f);
const Vec3 Vec3::LEFT = -Vec3::RIGHT;
const Vec3 Vec3::UP = Vec3(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::DOWN = -Vec3::UP;
const Vec3 Vec3::BACK = Vec3(0.0, 0.0, 1.0f);
const Vec3 Vec3::FORWARD = -Vec3::BACK;

float Math::Wrap(float x, float max) {
    return std::fmod(max + std::fmod(x, max), max);
}

float Math::Wrap(float x, float min, float max) {
    return min + Math::Wrap(x - min, max - min);
}

double Math::Wrap(double x, double max) {
    return std::fmod(max + std::fmod(x, max), max);
}

double Math::Wrap(double x, double min, double max) {
    return min + Math::Wrap(x - min, max - min);
}
