#pragma once

#include "logs/log.h"
#include "vulkan/vulkan.h"

#include <string>

namespace core::vk
{

class DebugUtils final
{
public:
    DebugUtils(
            const VkInstance& instance,
            VkDebugUtilsMessageSeverityFlagsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type);
    ~DebugUtils();

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData);

    VkResult createDebugUtilsMessengerEXT(
            VkInstance m_instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pMessenger);
    void destroyDebugUtilsMessengerEXT(
            VkInstance m_instance,
            VkDebugUtilsMessengerEXT messenger,
            const VkAllocationCallbacks* pAllocator);

    static const char* debugAnnotateObjectToString(
            const VkObjectType object_type);

private:
    logs::Logger m_Log;
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
    const VkInstance& m_Instance;
};

} // namespace render::vk

