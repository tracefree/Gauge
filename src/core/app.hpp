#pragma once

#include <string>

namespace Gauge {

    struct App {
    public:
        std::string name;

        void initialize();

    protected:
        bool quit_requested {false};
    
    public:
        void quit();

        virtual int run() = 0;
        virtual void update() {}
    };

}