#include "renderer.hpp"
#include <cassert>

using namespace Gauge;


uint Renderer::get_frames_in_flight() const { return max_frames_in_flight; }

void Renderer::set_frames_in_flight(uint p_max_frames_in_flight) {
    assert(!initialized);
    max_frames_in_flight = p_max_frames_in_flight;
}