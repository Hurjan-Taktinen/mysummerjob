#include "config/config.h"

#include <spdlog/fmt/ostr.h>
#include <filesystem>

namespace config
{

void Config::init(std::string_view optionalConfigPath)
{
    if(!optionalConfigPath.empty()
       && std::filesystem::exists(optionalConfigPath))
    {
        INFO("Optional config file given ({})", optionalConfigPath);
        m_ConfigFilePath = optionalConfigPath;
    }

    mINI::INIFile file(m_ConfigFilePath);
    if(!file.read(m_IniStruct))
    {
        WARNING("[Config] Failed to open config file, generating one with "
                "defaults");
        // TODO: do it actually
    }

    auto fromchars = [](const std::string_view src, auto& dst) {
        std::from_chars(&src.front(), &src.back(), dst);
    };

    INFO("[Config] Reading config, {} keys", m_IniStruct.size());

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

        INFO("[Config] base::width {}", config.width);
        INFO("[Config] base::height {}", config.height);

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

        INFO("[Config] vulkan::vsync {}", config.vsync);
        INFO("[Config] vulkan::validationlayers {}",
             config.enableValidationLayers);
        INFO("[Config] vulkan::debugutils {}", config.enableDebugUtils);
        m_VulkanConfig = config;
    }
}

} // namespace config

