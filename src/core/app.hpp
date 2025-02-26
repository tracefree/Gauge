#pragma once

#include "../renderer/renderer.hpp"
#include <string>

namespace Gauge {

    struct App {
    public:
        std::string name {"Gauge App"};

        void initialize();

    protected:
        bool quit_requested {false};
    
    public:
        void quit();

        virtual int run() = 0;
        virtual void update() {}
    };
}