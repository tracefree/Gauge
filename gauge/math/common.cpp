#include "common.hpp"

using namespace Gauge;

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
