#pragma once

#include <gauge/common.hpp>
#include <gauge/core/string_id.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <cstddef>

namespace Gauge {
struct Texture {
    unsigned char* data{};
    uint width{};
    uint height{};
    uint number_channels = 4;

    bool mipmapped{};
    bool use_srgb = false;

    size_t GetSize() const;
    static Result<Texture> FromFile(const std::string& p_path);

    ~Texture();

    // --- Resource interface ---
    static Texture Load(StringID p_id) {
        return FromFile(p_id).value();
    }
    void Unload();
};

}  // namespace Gauge