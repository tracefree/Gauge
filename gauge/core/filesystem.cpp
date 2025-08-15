#include "filesystem.hpp"

#include <gauge/common.hpp>

#include <fstream>
#include <ios>
#include <vector>

using namespace Gauge;

Result<std::vector<char>>
FileSystem::ReadFile(const std::string& p_path) {
    std::ifstream file(p_path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return Error("Failed to open file");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}