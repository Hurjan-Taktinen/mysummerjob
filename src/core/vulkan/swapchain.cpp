#include "swapchain.h"

#include "core/vulkan/utils.h"

#include <algorithm>
#include <cassert>

namespace core::vk
{

// ----------------------------------------------------------------------------
//
//

Swapchain::Swapchain(VkInstance instance, Device* device) :
    m_Log(logs::Log::create("Vulkan Swapchain")),
    m_Device(device),
    m_Instance(instance)
{
    auto window = m_Device->getGLFWwindow();
    assert(window);
    VK_CHECK(glfwCreateWindowSurface(
            m_Instance, window.get(), nullptr, &m_Surface));

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_Device->getPhysicalDevice(), m_Surface, &m_SurfaceCapabilities);

    // TODO improve, take just type of T, not instance
    auto deviceQuery = [device = m_Device, surface = m_Surface]<typename T>(
                               auto&& Func, T t) {
        uint32_t count = 0;
        Func(device->getPhysicalDevice(), surface, &count, nullptr);
        assert(count > 0);
        std::vector<decltype(t)> res(count);
        Func(device->getPhysicalDevice(), surface, &count, res.data());
        return res;
    };

    m_PresentModeList = deviceQuery(
            vkGetPhysicalDeviceSurfacePresentModesKHR, VkPresentModeKHR {});
    m_SurfaceFormatList = deviceQuery(
            vkGetPhysicalDeviceSurfaceFormatsKHR, VkSurfaceFormatKHR {});
}

// ----------------------------------------------------------------------------
//
//

Swapchain::~Swapchain()
{
    if(m_Depth.view)
    {
        vkDestroyImageView(m_Device->getLogicalDevice(), m_Depth.view, nullptr);
    }
    if(m_Depth.image)
    {
        vmaDestroyImage(
                m_Device->getAllocator(), m_Depth.image, m_Depth.memory);
    }
    for(auto& framebuffer : m_FrameBuffers)
    {
        if(framebuffer)
        {
            vkDestroyFramebuffer(
                    m_Device->getLogicalDevice(), framebuffer, nullptr);
        }
    }
    for(auto& view : m_ImageViews)
    {
        if(view)
        {
            vkDestroyImageView(m_Device->getLogicalDevice(), view, nullptr);
        }
    }
    if(m_Swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(
                m_Device->getLogicalDevice(), m_Swapchain, nullptr);
    }
    if(m_Surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

VkResult Swapchain::acquireNextImage(
        VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
{
    return vkAcquireNextImageKHR(
            m_Device->getLogicalDevice(),
            m_Swapchain,
            UINT64_MAX,
            presentCompleteSemaphore,
            VK_NULL_HANDLE,
            imageIndex);
}

// ----------------------------------------------------------------------------
//
//

VkResult Swapchain::queuePresent(
        VkQueue queue, uint32_t* imageIndex, VkSemaphore* waitSemaphore)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = imageIndex;
    presentInfo.pResults = nullptr;

    return vkQueuePresentKHR(queue, &presentInfo);
}

// ----------------------------------------------------------------------------
//
//

void Swapchain::create(bool vsync)
{
    assert(m_Surface);

    // Update surfaceCapabilities after resize
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_Device->getPhysicalDevice(), m_Surface, &m_SurfaceCapabilities);

    uint32_t minImageCount = m_SurfaceCapabilities.minImageCount;
    m_Log->info("Swapchain minImageCount({})", minImageCount);

    // Search for preferred format, if not found, pick first one in the list.
    assert(m_SurfaceFormatList.size() > 0);
    const VkFormat preferredSurfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;

    auto res = std::find_if(
            m_SurfaceFormatList.begin(),
            m_SurfaceFormatList.end(),
            [surfaceformat = preferredSurfaceFormat](const auto& format) {
                return format.format == surfaceformat;
            });

    if(res != m_SurfaceFormatList.end())
    {
        m_SurfaceFormat = res->format;
        m_SurfaceColorSpace = res->colorSpace;
    }
    else
    {
        m_SurfaceFormat = m_SurfaceFormatList[0].format;
        m_SurfaceColorSpace = m_SurfaceFormatList[0].colorSpace;
    }

    // FIFO is guaranteed by spec
    m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if(!vsync)
    {
        for(const auto& mode : m_PresentModeList)
        {
            if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                m_PresentMode = mode;
                minImageCount += 1;
                break;
            }
        }
    }

    auto extent = [log = m_Log,
                   device = m_Device,
                   capabilities = m_SurfaceCapabilities] {
        if(capabilities.currentExtent.width
           != std::numeric_limits<uint32_t>::max())
        {
            log->info(
                    "CurrentExtent({}, {})",
                    capabilities.currentExtent.width,
                    capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }
        else
        {
            int width = 0, height = 0;
            glfwGetFramebufferSize(
                    device->getGLFWwindow().get(), &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)};

            actualExtent.width = std::max(
                    capabilities.minImageExtent.width,
                    std::min(
                            capabilities.maxImageExtent.width,
                            actualExtent.width));

            actualExtent.height = std::max(
                    capabilities.minImageExtent.height,
                    std::min(
                            capabilities.maxImageExtent.height,
                            actualExtent.height));

            log->info(
                    "ActualExtent({}, {})",
                    actualExtent.width,
                    actualExtent.height);

            return actualExtent;
        }
    }();

    m_Extent = extent;

    // 0 is also valid, unlimited maximagecount
    assert(m_SurfaceCapabilities.maxImageCount == 0
           || m_SurfaceCapabilities.maxImageCount >= minImageCount);

    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(
            m_Device->getPhysicalDevice(), m_SurfaceFormat, &formatProperties);

    if((formatProperties.optimalTilingFeatures
        & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR)
       || (formatProperties.optimalTilingFeatures
           & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
    {
        imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    VkSwapchainKHR oldSwapchain = m_Swapchain;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = minImageCount;
    createInfo.imageFormat = m_SurfaceFormat;
    createInfo.imageColorSpace = m_SurfaceColorSpace;
    createInfo.imageExtent = m_Extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = imageUsage;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = m_PresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    VK_CHECK(vkCreateSwapchainKHR(
            m_Device->getLogicalDevice(), &createInfo, nullptr, &m_Swapchain));

    // TODO: remove
    m_ImageCount = minImageCount;

    // Clean up old swapchain and associated views
    if(oldSwapchain)
    {
        for(auto& view : m_ImageViews)
        {
            vkDestroyImageView(m_Device->getLogicalDevice(), view, nullptr);
        }

        vkDestroySwapchainKHR(
                m_Device->getLogicalDevice(), oldSwapchain, nullptr);
    }

    // Get handles for new swapchain images
    vkGetSwapchainImagesKHR(
            m_Device->getLogicalDevice(), m_Swapchain, &m_ImageCount, nullptr);
    m_Images.resize(m_ImageCount);

    vkGetSwapchainImagesKHR(
            m_Device->getLogicalDevice(),
            m_Swapchain,
            &m_ImageCount,
            m_Images.data());

    // Create image views
    m_ImageViews.resize(m_ImageCount);
    for(uint32_t i = 0; i < m_ImageCount; ++i)
    {
        m_Device->createImageView(
                m_Images[i],
                m_SurfaceFormat,
                VK_IMAGE_ASPECT_COLOR_BIT,
                &m_ImageViews[i]);
    }

    // (re)create Depth buffer
    if(m_Depth.image)
    {
        vkDestroyImageView(m_Device->getLogicalDevice(), m_Depth.view, nullptr);
        vmaDestroyImage(
                m_Device->getAllocator(), m_Depth.image, m_Depth.memory);
    }

    m_Device->createImage(
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_Depth.format,
            VK_IMAGE_TILING_OPTIMAL,
            m_Extent,
            &m_Depth.image,
            &m_Depth.memory,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

    m_Device->createImageView(
            m_Depth.image,
            m_Depth.format,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            &m_Depth.view);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_Depth.image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkCommandBuffer commandBuffer = m_Device->createCommandBuffer(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

    m_Device->flushCommandBuffer(
            commandBuffer, m_Device->getGraphicsQueue(), true);
}

// ----------------------------------------------------------------------------
//
//

void Swapchain::createFrameBuffers(VkRenderPass renderpass)
{

    m_FrameBuffers.resize(m_ImageCount);

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.renderPass = renderpass;
    createInfo.width = m_Extent.width;
    createInfo.height = m_Extent.height;
    createInfo.layers = 1;

    for(uint32_t i = 0; i < m_ImageCount; ++i)
    {
        std::array<VkImageView, 2> attachments = {
                m_ImageViews[i], m_Depth.view};
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();

        VK_CHECK(vkCreateFramebuffer(
                m_Device->getLogicalDevice(),
                &createInfo,
                nullptr,
                &m_FrameBuffers[i]));
    }
}

// ----------------------------------------------------------------------------
//
//

void Swapchain::destroyFrameBuffers()
{
    for(auto& fb : m_FrameBuffers)
    {
        vkDestroyFramebuffer(m_Device->getLogicalDevice(), fb, nullptr);
    }
}

} // namespace core::vk
