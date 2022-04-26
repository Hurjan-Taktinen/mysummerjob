#pragma once

#include "core/vulkan/device.h"
#include "core/texture/gli.h"
#include "logs/log.h"

#include "vulkan/vulkan.h"

#include <string>
#include <optional>

namespace core::texture
{
class Texture
{
public:
    virtual ~Texture() = default;

    virtual void loadFromFile(
            vk::Device* device,
            const std::string& file,
            VkFormat format,
            VkImageUsageFlags imageUsage,
            VkImageLayout imageLayout) = 0;

    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VmaAllocation memory = VK_NULL_HANDLE;
    VkImageLayout layout;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 0;
    uint32_t layerCount = 0;

    std::optional<VkDescriptorImageInfo> descriptor = {};
    VkSampler sampler = VK_NULL_HANDLE;

protected:
    void updateDescriptor()
    {
        descriptor = VkDescriptorImageInfo{sampler, view, layout};
    }

    void clean()
    {
        if(view)
            vkDestroyImageView(device->getLogicalDevice(), view, nullptr);
        if(image)
            vmaDestroyImage(device->getAllocator(), image, memory);
        if(sampler)
            vkDestroySampler(device->getLogicalDevice(), sampler, nullptr);
    }

    inline static logs::Logger m_Log;
    vk::Device* device;
};

class Texture2d final : public Texture
{
public:
    Texture2d()
    {
        if(!m_Log)
        {
            m_Log = logs::Log::create("Texture2d");
        }
    }

    ~Texture2d() { clean(); }

    void loadFromFile(
            vk::Device* device,
            const std::string& file,
            VkFormat format,
            VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout imageLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) override;

    void loadFromBuffer(
            vk::Device* device,
            void* buffer,
            VkDeviceSize bufferSize,
            VkFormat format,
            uint32_t width,
            uint32_t height,
            VkFilter filter = VK_FILTER_LINEAR,
            VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout imageLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};
} // namespace core::texture
