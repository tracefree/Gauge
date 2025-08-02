#pragma once

#include <expected>
#include <vector>
namespace Gauge {

namespace FileSystem {

std::expected<std::vector<char>, std::string> ReadFile(const std::string& p_path);

}

}  // namespace Gauge