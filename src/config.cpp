#include "config/config.h"

#include <spdlog/fmt/ostr.h>
#include <filesystem>

namespace config
{

void Config::init(std::string_view optionalConfigPath)
{
    m_Log = logs::Log::create("Config");

    if(!optionalConfigPath.empty()
       && std::filesystem::exists(optionalConfigPath))
    {
        m_Log->info("Optional config file given ({})", optionalConfigPath);
        m_ConfigFilePath = optionalConfigPath;
    }

    mINI::INIFile file(m_ConfigFilePath);
    if(!file.read(m_IniStruct))
    {
        m_Log->warn("Failed to open config file, generating one with "
                    "defaults");
        // TODO: do it actually
    }

    auto fromchars = [](const std::string& src, auto& dst) {
        std::from_chars(src.data(), src.data() + src.size(), dst);
    };

    m_Log->info("Configuration file found at {}", m_ConfigFilePath.size());
    m_Log->info("Found {} keys, parsing...", m_IniStruct.size());

    { // BaseConfig
        BaseConfig config;
        if(m_IniStruct.has("base"))
        {
            auto section = m_IniStruct.get("base");
            fromchars(section.get("width"), config.width);
            fromchars(section.get("height"), config.height);
        }
        else
        {
            // use default values given in struct declarations
        }

        m_Log->info("base::width {}", config.width);
        m_Log->info("base::height {}", config.height);

        m_BaseConfig = config;
    }

    { // VkConfig
        VulkanConfig config;
        if(m_IniStruct.has("vulkan"))
        {
            auto section = m_IniStruct.get("vulkan");
            fromchars(section["vsync"], config.vsync);
            fromchars(
                    section["validationlayers"], config.enableValidationLayers);
            fromchars(section["debugutils"], config.enableDebugUtils);
        }
        else
        {
            // use default values
        }

        m_Log->info("vulkan::vsync {}", config.vsync);
        m_Log->info(
                "vulkan::validationlayers {}",
                config.enableValidationLayers);
        m_Log->info("vulkan::debugutils {}", config.enableDebugUtils);
        m_VulkanConfig = config;
    }
}

} // namespace config

