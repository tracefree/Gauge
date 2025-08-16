#pragma once

#include <gauge/common.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <cstddef>

namespace Gauge {
struct Texture {
    unsigned char* data{};
    uint width{};
    uint height{};
    uint number_channels = 4;

    bool mipmapped{};
    bool use_srgb = true;

    size_t GetSize() const;
    static Result<Texture> FromFile(const std::string& p_path);

    ~Texture();
};

}  // namespace Gauge