#pragma once

#include <gauge/common.hpp>

#include <vector>

namespace Gauge {

namespace FileSystem {

Result<std::vector<char>> ReadFile(const std::string& p_path);

}

}  // namespace Gauge