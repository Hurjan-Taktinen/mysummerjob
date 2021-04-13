#pragma once

#include "core/vulkan/device.h"

#include "vulkan/vulkan.h"
#include "logs/log.h"

#include <vector>

namespace core::vk
{

class Swapchain
{
public:
    Swapchain(VkInstance instance, Device* device);
    ~Swapchain();

    [[nodiscard]] auto getSurface() const { return m_Surface; }
    [[nodiscard]] auto getImageCount() const { return m_ImageCount; }
    [[nodiscard]] auto getImageFormat() const { return m_SurfaceFormat; }
    [[nodiscard]] auto getFrameBuffer(size_t i) const
    {
        return m_FrameBuffers[i];
    }
    [[nodiscard]] auto getExtent() const { return m_Extent; }

    [[nodiscard]] VkResult acquireNextImage(
            VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
    [[nodiscard]] VkResult queuePresent(
            VkQueue queue, uint32_t* imageIndex, VkSemaphore* waitSemaphore);
    void create(bool vsync);
    void createFrameBuffers(VkRenderPass renderpass);
    void destroyFrameBuffers();

    [[nodiscard]] const auto& getSurfaceCapabilities() const
    {
        return m_SurfaceCapabilities;
    }

private:

    logs::Logger m_Log;
    Device* m_Device;
    VkInstance m_Instance;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
    VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D m_Extent = {0, 0};
    VkFormat m_SurfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR m_SurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32_t m_ImageCount = 0;

    struct
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VmaAllocation memory = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_D32_SFLOAT;
    } m_Depth;

    std::vector<VkImage> m_Images;
    std::vector<VkFramebuffer> m_FrameBuffers;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkPresentModeKHR> m_PresentModeList;
    std::vector<VkSurfaceFormatKHR> m_SurfaceFormatList;
};

} // namespace core::vk

