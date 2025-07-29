#include "renderer.hpp"

#include <cassert>

using namespace Gauge;

uint Renderer::GetFramesInFlight() const {
    return max_frames_in_flight;
}

void Renderer::SetFramesInFlight(uint p_max_frames_in_flight) {
    assert(!initialized);
    max_frames_in_flight = p_max_frames_in_flight;
}