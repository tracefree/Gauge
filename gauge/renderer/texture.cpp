#include "texture.hpp"

#include <filesystem>
#include <format>
#include <print>

#include "thirdparty/KTX-Software/include/ktx.h"
#include "thirdparty/stb/stb_image.h"

using namespace Gauge;

size_t Texture::GetSize() const {
    return width * height * 4;
}

Result<Texture> Texture::FromFile(const std::string& p_path) {
    Texture texture{};

    std::filesystem::path path(p_path);
    auto ktx_path = path.replace_extension("ktx2");
    if (std::filesystem::exists(ktx_path)) {
        std::println("Loading ktx2 file: {}", ktx_path.c_str());
        ktxTexture2* ktx_texture{};
        auto result = ktxTexture2_CreateFromNamedFile(ktx_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
        if (result != KTX_SUCCESS) {
            return std::unexpected(std::format("Could not create KTX2 texture {}. Error: {}", ktx_path.c_str(), ktxErrorString(result)));
        }
    }

    int w, h, ch;
    texture.data = stbi_load(p_path.c_str(), &w, &h, &ch, 4);
    if (!texture.data) {
        return Error(std::format("Could not load image file '{}'", p_path));
    }
    texture.width = w;
    texture.height = h;
    texture.number_channels = ch;
    return texture;
}

Texture::~Texture() {
    if (data != nullptr) {
        // TODO: Clean up
        // stbi_image_free(data);
    }
}