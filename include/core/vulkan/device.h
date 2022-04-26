#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "gpuopen/vkmemalloc.h"
#include "logs/log.h"

#include <memory>
#include <vector>
#include <string>

namespace core::vk
{

class Device final
{
public:
    explicit Device(
            VkPhysicalDevice gpu, const std::shared_ptr<GLFWwindow>& window);
    ~Device();

    Device() = delete;
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;

    void createLogicalDevice(
            const VkInstance& instance,
            std::vector<const char*> requestedExtensions,
            VkQueueFlags requestedQueueTypes);

    [[nodiscard]] const std::shared_ptr<GLFWwindow>& getGLFWwindow() const;
    [[nodiscard]] VkDevice getLogicalDevice() const { return m_LogicalDevice; }
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const
    {
        return m_PhysicalDevice;
    }
    [[nodiscard]] VmaAllocator getAllocator() const { return m_Allocator; }

    [[nodiscard]] uint32_t getGraphicsQueueFamily() const
    {
        return m_QueueFamilyIndices.graphics;
    }
    [[nodiscard]] uint32_t getComputeQueueFamily() const
    {
        return m_QueueFamilyIndices.compute;
    }
    [[nodiscard]] uint32_t getTransferQueueFamily() const
    {
        return m_QueueFamilyIndices.transfer;
    }

    [[nodiscard]] VkQueue getGraphicsQueue() const { return m_Queue.graphics; }
    [[nodiscard]] VkQueue getComputeQueue() const { return m_Queue.compute; }
    [[nodiscard]] VkQueue getPresentQueue() const { return m_Queue.present; }
    [[nodiscard]] VkQueue getTransferQueue() const { return m_Queue.transfer; }

    [[nodiscard]] VkCommandPool getGraphicsCommandPool() const
    {
        return m_CommandPools.graphics;
    }
    [[nodiscard]] VkCommandPool getComputeCommandPool() const
    {
        return m_CommandPools.compute;
    }
    [[nodiscard]] VkCommandPool getTransferCommandPool() const
    {
        return m_CommandPools.transfer;
    }

    VkCommandBuffer createCommandBuffer(
            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            VkQueueFlags queueType = VK_QUEUE_GRAPHICS_BIT,
            bool begin = true);

    void beginCommandBuffer(
            VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags = 0);

    void flushCommandBuffer(
            VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

    void createBuffer(
            VkBufferUsageFlags usage,
            VmaMemoryUsage vmaUsage,
            VkMemoryPropertyFlags properties,
            VkDeviceSize size,
            VkBuffer* buffer,
            VmaAllocation* bufferMemory,
            void* data = nullptr);

    void createBufferOnGPU(
            VkBufferUsageFlags usage,
            VkDeviceSize size,
            VkBuffer* buffer,
            VmaAllocation* bufferMemory,
            void* data);

    void createImageOnGPU(
            VkImageUsageFlags usage,
            VkDeviceSize size,
            VkFormat format,
            VkImageLayout layout,
            VkExtent2D extent,
            VkImage* image,
            VmaAllocation* imageMemory,
            void* data);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void copyBufferToImage(
            VkBuffer srcBuffer,
            VkImage dstImage,
            VkExtent2D extent,
            VkImageLayout layout,
            VkDeviceSize size);

    void createImageView(
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspect,
            VkImageView* view);

    void createImage(
            VkImageUsageFlags usage,
            VmaMemoryUsage vmaMemoryUsage,
            VkFormat format,
            VkImageTiling tiling,
            VkExtent2D extent,
            VkImage* image,
            VmaAllocation* memory,
            VmaAllocationCreateFlags vmaFlags = 0,
            VkImageCreateFlags imageFlags = 0,
            uint32_t mipLevels = 1,
            uint32_t arrayLayers = 1);

    void createSampler(VkSampler* sampler);

    VkPipelineShaderStageCreateInfo loadShaderFromFile(
            const std::string& path, VkShaderStageFlagBits stage);
    uint32_t findMemoryType(
            uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
    [[nodiscard]] uint32_t getQueueFamilyIndex(
            VkQueueFlagBits queueFlags) const;
    [[nodiscard]] VkCommandPool createCommandPool(
            uint32_t queueFamilyIndex,
            VkCommandPoolCreateFlags poolFlags) const;

    logs::Logger m_Log;
    std::shared_ptr<GLFWwindow> m_Window;
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
    VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
    VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;

    struct
    {
        VkQueue graphics = VK_NULL_HANDLE;
        VkQueue compute = VK_NULL_HANDLE;
        VkQueue present = VK_NULL_HANDLE;
        VkQueue transfer = VK_NULL_HANDLE;
    } m_Queue;

    struct
    {
        uint32_t graphics = -1;
        uint32_t compute = -1;
        uint32_t transfer = -1;
    } m_QueueFamilyIndices;

    struct
    {
        VkCommandPool graphics = VK_NULL_HANDLE;
        VkCommandPool compute = VK_NULL_HANDLE;
        VkCommandPool transfer = VK_NULL_HANDLE;
    } m_CommandPools;
};
} // namespace core::vk
