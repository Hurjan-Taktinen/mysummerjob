#include "utils/stringutils.h"

namespace utils
{

auto getDirectory(const std::string& input) -> std::string
{
    const auto path = std::filesystem::path(input);
    const auto parentPath = path.parent_path();

    return parentPath.string();
}

auto getExtension(const std::string& input) -> std::string
{
    const auto path = std::filesystem::path(input);
    std::string ext = path.extension();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

auto getFilename(const std::string& input) -> std::string
{
    const auto path = std::filesystem::path(input);
    return path.filename();
}

} // namespace utils
