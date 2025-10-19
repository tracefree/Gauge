#include "texture.hpp"

#include <filesystem>
#include <format>
#include <print>

#include "ktx.h"
#include "thirdparty/stb/stb_image.h"

using namespace Gauge;

size_t Texture::GetSize() const {
    return width * height * 4;
}

Result<Texture> Texture::FromFile(const std::string& p_path) {
    std::filesystem::path path(p_path);
    auto ktx_path = path;
    ktx_path.replace_extension("ktx2");
    if (std::filesystem::exists(ktx_path)) {
        return Texture::LoadKTX(ktx_path);
    }
    return LoadSTB(path);
}

Result<Texture> Texture::LoadKTX(const std::filesystem::path p_path) {
    std::println("Loading ktx2 file: {}", p_path.c_str());
    Texture texture{};
    texture.ktx_texture = new ktxTexture2;
    auto result = ktxTexture2_CreateFromNamedFile(p_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture.ktx_texture);
    if (result != KTX_SUCCESS) {
        return Error(std::format("Could not load KTX texture {}. Error: {}", p_path.c_str(), ktxErrorString(result)));
    }

    auto color_model = ktxTexture2_GetColorModel_e(texture.ktx_texture);
    ktx_transcode_fmt_e texture_format;
    if (color_model != KHR_DF_MODEL_RGBSDA) {
        if (color_model == KHR_DF_MODEL_UASTC) {
            texture_format = KTX_TTF_ASTC_4x4_RGBA;
        } else if (color_model == KHR_DF_MODEL_ETC1S) {
            texture_format = KTX_TTF_ETC;
        } else {
            return Error("No suitable transcoding format supported");
        }
        result = ktxTexture2_TranscodeBasis(texture.ktx_texture, KTX_TTF_BC7_RGBA, 0);
        if (result != KTX_SUCCESS) {
            return Error(std::format("Could not transcode texture to format {}. Error: {}", (int)texture_format, ktxErrorString(result)));
        }
    }

    return texture;
}

Result<Texture> Texture::LoadSTB(const std::filesystem::path p_path) {
    Texture texture{};
    int width, height, number_channels;
    texture.data = stbi_load(p_path.c_str(), &width, &height, &number_channels, 4);
    if (!texture.data) {
        return Error(std::format("Could not load image file '{}'", p_path.c_str()));
    }
    texture.width = width;
    texture.height = height;
    texture.number_channels = number_channels;
    return texture;
}

Texture::~Texture() {
    if (data != nullptr) {
        // TODO: Clean up
        // stbi_image_free(data);
        if (ktx_texture != nullptr) {
            // ktxTexture2_Destroy(ktx_texture);
        }
    }
}