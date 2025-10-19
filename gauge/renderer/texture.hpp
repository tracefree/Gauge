#pragma once

#include <gauge/common.hpp>
#include <gauge/core/string_id.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include "thirdparty/KTX-Software/include/ktx.h"

#include <cstddef>
#include <filesystem>

namespace Gauge {
struct Texture {
    unsigned char* data{};
    ktxTexture2* ktx_texture{};
    uint width{};
    uint height{};
    uint number_channels = 4;

    bool mipmapped{};
    bool use_srgb = false;

    size_t GetSize() const;
    static Result<Texture> FromFile(const std::string& p_path);
    static Result<Texture> LoadKTX(const std::filesystem::path p_path);
    static Result<Texture> LoadSTB(const std::filesystem::path p_path);

    ~Texture();

    // --- Resource interface ---
    static Texture Load(StringID p_id) {
        auto result = FromFile(p_id);
        CHECK(result);
        return result.value();
    }
    void Unload();
};

}  // namespace Gauge