#pragma once

#include <gauge/renderer/renderer.hpp>

#include <memory>
#include <string>

namespace Gauge {

struct App {
   public:
    std::string name{"Gauge App"};
    std::unique_ptr<Renderer> renderer = nullptr;
    void Initialize();

   protected:
    bool quit_requested{false};

   public:
    void Quit();

    virtual int Run() = 0;
    virtual void Update() = 0;
};

}  // namespace Gauge