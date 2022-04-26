#include "debugutils.h"

#include "logs/log.h"
#include "core/vulkan/utils.h"

#include <sstream>
#include <iostream>

namespace core::vk
{

DebugUtils::DebugUtils(
        const VkInstance& instance,
        VkDebugUtilsMessageSeverityFlagsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type) :
    m_Log(logs::Log::create("Vulkan DebugUtils")), m_Instance(instance)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.messageSeverity = severity;
    createInfo.messageType = type;
    createInfo.pfnUserCallback = debug_messenger_callback;
    createInfo.pUserData = nullptr;

    VK_CHECK(createDebugUtilsMessengerEXT(
            instance, &createInfo, nullptr, &m_Messenger));
}

DebugUtils::~DebugUtils()
{
    if(m_Messenger != VK_NULL_HANDLE)
    {
        destroyDebugUtilsMessengerEXT(m_Instance, m_Messenger, nullptr);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtils::debug_messenger_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
{
    (void)userData;
    std::ostringstream ss;

    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        ss << "GENERAL";
    }
    else if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        ss << "VALIDATION";
    }
    else if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        ss << "PERFORMANCE";
    }

    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        LGINFO("VERBOSE : {}", ss.str());
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        LGINFO("INFO : {}", ss.str());
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LGWARNING("WARNING : {}", ss.str());
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LGCRITICAL("ERROR : {}", ss.str());
    }

    ss << "Message ID [" << callbackData->messageIdNumber << "]";
    if(callbackData->pMessageIdName)
        ss << ", Message ID string : " << callbackData->pMessageIdName << "\n";
    ss << "Message : " << callbackData->pMessage;

    if(callbackData->objectCount > 0)
    {
        ss << "\n";
        ss << "Object count : " << callbackData->objectCount << "\n";
        for(uint32_t object = 0; object < callbackData->objectCount; ++object)
        {
            ss << "\tObject [" << object << "] - Type : "
               << debugAnnotateObjectToString(
                          callbackData->pObjects[object].objectType)
               << "\n\t"
               << "Value : "
               << (void*)(callbackData->pObjects[object].objectHandle);
            if(callbackData->pObjects[object].pObjectName)
            {
                ss << callbackData->pObjects[object].pObjectName << "\n";
            }
        }
    }

    if(callbackData->cmdBufLabelCount > 0)
    {
        ss << "\n";
        ss << "Command buffer label count - " << callbackData->cmdBufLabelCount
           << "\n";
        for(uint32_t label = 0; label < callbackData->cmdBufLabelCount; ++label)
        {
            ss << "\tLabel [" << label << "] - "
               << callbackData->pCmdBufLabels[label].pLabelName << " { "
               << callbackData->pCmdBufLabels[label].color[0] << ", "
               << callbackData->pCmdBufLabels[label].color[1] << ", "
               << callbackData->pCmdBufLabels[label].color[2] << ", "
               << callbackData->pCmdBufLabels[label].color[3] << " }\n";
        }
    }
    std::cout << ss.str();
    std::cout << std::endl;
    return VK_FALSE;
}

VkResult DebugUtils::createDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pMessenger)
{
    static const auto func =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(
                            instance, "vkCreateDebugUtilsMessengerEXT"));

    if(func)
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DebugUtils::destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT messenger,
        const VkAllocationCallbacks* pAllocator)
{
    static const auto func =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(
                            instance, "vkDestroyDebugUtilsMessengerEXT"));
    func(instance, messenger, pAllocator);
}

const char* DebugUtils::debugAnnotateObjectToString(
        const VkObjectType object_type)
{
    // clang-format off
    switch(object_type)
    {
    case VK_OBJECT_TYPE_INSTANCE: return "VkInstance";
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE: return "VkPhysicalDevice";
    case VK_OBJECT_TYPE_DEVICE: return "VkDevice";
    case VK_OBJECT_TYPE_QUEUE: return "VkQueue";
    case VK_OBJECT_TYPE_SEMAPHORE: return "VkSemaphore";
    case VK_OBJECT_TYPE_COMMAND_BUFFER: return "VkCommandBuffer";
    case VK_OBJECT_TYPE_FENCE: return "VkFence";
    case VK_OBJECT_TYPE_DEVICE_MEMORY: return "VkDeviceMemory";
    case VK_OBJECT_TYPE_BUFFER: return "VkBuffer";
    case VK_OBJECT_TYPE_IMAGE: return "VkImage";
    case VK_OBJECT_TYPE_EVENT: return "VkEvent";
    case VK_OBJECT_TYPE_QUERY_POOL: return "VkQueryPool";
    case VK_OBJECT_TYPE_BUFFER_VIEW: return "VkBufferView";
    case VK_OBJECT_TYPE_IMAGE_VIEW: return "VkImageView";
    case VK_OBJECT_TYPE_SHADER_MODULE: return "VkShaderModule";
    case VK_OBJECT_TYPE_PIPELINE_CACHE: return "VkPipelineCache";
    case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "VkPipelineLayout";
    case VK_OBJECT_TYPE_RENDER_PASS: return "VkRenderPass";
    case VK_OBJECT_TYPE_PIPELINE: return "VkPipeline";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "VkDescriptorSetLayout";
    case VK_OBJECT_TYPE_SAMPLER: return "VkSampler";
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "VkDescriptorPool";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "VkDescriptorSet";
    case VK_OBJECT_TYPE_FRAMEBUFFER: return "VkFramebuffer";
    case VK_OBJECT_TYPE_COMMAND_POOL: return "VkCommandPool";
    case VK_OBJECT_TYPE_SURFACE_KHR: return "VkSurfaceKHR";
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "VkSwapchainKHR";
    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: return "VkDebugReportCallbackEXT";
    case VK_OBJECT_TYPE_DISPLAY_KHR: return "VkDisplayKHR";
    case VK_OBJECT_TYPE_DISPLAY_MODE_KHR: return "VkDisplayModeKHR";
    // case VK_OBJECT_TYPE_OBJECT_TABLE_NVX: return "VkObjectTableNVX";
    // case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX: return "VkIndirectCommandsLayoutNVX";
    case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR: return "VkDescriptorUpdateTemplateKHR";
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "VkDebugUtilsMessengerEXT";
    default: return "Unknown Type";
    }
    // clang-format on
}

} // namespace core::vk
