#pragma once

#include <SDL3/SDL_video.h>


namespace Gauge {
    struct Renderer {
    protected:
        bool initialized = false;
        uint max_frames_in_flight = 3;
    
    public:
        virtual bool initialize(SDL_Window* p_sdl_window) { return false; };
        virtual void draw() {};
        virtual void create_surface(SDL_Window* window) {};

        Renderer() {}
        ~Renderer() {}

        uint get_frames_in_flight() const;
        void set_frames_in_flight(uint p_max_frames_in_flight);
    };
}