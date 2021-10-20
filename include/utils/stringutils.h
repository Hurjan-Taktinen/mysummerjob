#pragma once

#include <algorithm>
#include <filesystem>
#include <string>

namespace utils
{

auto getDirectory(const std::string& input) -> std::string;
auto getExtension(const std::string& input) -> std::string;
auto getFilename(const std::string& input) -> std::string;

} // namespace utils
