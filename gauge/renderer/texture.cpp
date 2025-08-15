#include "texture.hpp"

#include "thirdparty/stb/stb_image.h"

using namespace Gauge;

size_t Texture::GetSize() const {
    return width * height * 4;
}

Result<Texture> Texture::FromFile(const std::string& p_path) {
    Texture texture{};
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