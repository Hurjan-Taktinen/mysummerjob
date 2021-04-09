#pragma once

#include "mini/ini.h"
#include "logs/log.h"

#include <charconv>
#include <fmt/format.h>
#include <string>
#include <string_view>

namespace config
{

struct BaseConfig
{
    // Display resolution
    int width = 800;
    int height = 800;
};

struct VulkanConfig
{
    int enableValidationLayers = false;
    int enableDebugUtils = false;
    int vsync = false;
};

class Config final
{
public:
    static void init(std::string_view optionalConfigPath);

    [[nodiscard]] static auto getVulkanConfig() { return m_VulkanConfig; }
    [[nodiscard]] static auto getBaseConfig() { return m_BaseConfig; }

private:
    inline static std::string m_ConfigFilePath = "data/config.ini";
    inline static mINI::INIStructure m_IniStruct;

    inline static BaseConfig m_BaseConfig;
    inline static VulkanConfig m_VulkanConfig;
};

} // namespace config
