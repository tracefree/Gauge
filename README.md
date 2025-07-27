# Gauge Engine

This is an experiment in trying out a better way to structure a code base for a game engine and may or may not end up succeeding my current engine [Propser](https://github.com/tracefree/prosper). Specifically, I am switching to [VkBootstrap](https://github.com/charles-lunarg/vk-bootstrap) for initializing Vulkan and testing working with Vulkan's C API instead of using VulkanHpp.

Prosper started by following [vkguide.dev](https://vkguide.dev/). This time I would like to take the time and apply what I learned and plan ahead a bit more, and be stricter about separating areas of responsibility, clean code, etc. Right now all it does is create an empty window and initialize Vulkan - in the future I may port all the features of Prosper over, or refactor prosper itself.

## Building

Requirements:
- CMake
- Git
- Vulkan
- SDL3

Recommended:
- Ninja
- Ccache

To build both the engine library run:

```
git clone https://github.com/tracefree/Gauge.git
cd Gauge
cmake . -B build -G Ninja
cmake --build build
```
This builds a static library file, `libgauge.a`. An example application making use of the library is not currently included but will follow.

## License

The engine is available under the [MIT License](LICENSE.md).
