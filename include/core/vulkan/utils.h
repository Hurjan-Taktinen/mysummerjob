#pragma once

#include <string>
#include <sstream>
#include "vulkan/vulkan.h"

namespace core::vk::utils
{

std::string errorString(VkResult result);
std::string deviceType(VkPhysicalDeviceType deviceType);

} // namespace core::vk::utils

#define VK_CHECK(f)                                                            \
    {                                                                          \
        VkResult res = (f);                                                    \
        if(res != VK_SUCCESS)                                                  \
        {                                                                      \
            std::stringstream ss;                                              \
            ss << "Fatal : VkResult is \""                                     \
               << core::vk::utils::errorString(res) << "\" in " << __FILE__    \
               << " at line " << __LINE__ << std::endl;                        \
            throw std::runtime_error(ss.str());                                \
        }                                                                      \
    }

