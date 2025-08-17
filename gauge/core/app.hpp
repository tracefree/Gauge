#pragma once

#include <chrono>
#include <gauge/renderer/renderer.hpp>

#include <memory>
#include <string>

namespace Gauge {

struct App {
   public:
    std::string name{"Gauge App"};
    std::unique_ptr<Renderer> renderer;
    std::chrono::steady_clock::time_point start_time;

   protected:
    bool quit_requested{false};

   public:
    void Initialize();
    void Quit();

    virtual int Run() = 0;
    virtual void Update() = 0;
};

}  // namespace Gauge