#pragma once

#include <gauge/renderer/renderer.hpp>

#include <memory>
#include <string>

namespace Gauge {

struct App {
   public:
    std::string name{"Gauge App"};
    std::unique_ptr<Renderer> renderer;
    void initialize();

   protected:
    bool quit_requested{false};

   public:
    void quit();

    virtual int run() = 0;
    virtual void update() {}
};
}  // namespace Gauge