# Gauge Engine

This is the successor / ongoing rewrite of my previous 3D engine [Propser](https://github.com/tracefree/prosper). Currently implemented features are:

- glTF model and texture loading
- Scene tree with transform hierarchy
- Using the "bindless" paradigm
- Separate engine into core library and application

## Building

Requirements:
- CMake
- Git
- Vulkan
- SDL3

Recommended:
- Ninja
- Ccache

To build the engine library run:

```
git clone https://github.com/tracefree/Gauge.git
cd Gauge
cmake . -B build -G Ninja
cmake --build build
```
This builds a static library file, `libgauge.a`. An example application making use of the library is not currently included but will follow.

## License

The engine is available under the [MIT License](LICENSE.md).
