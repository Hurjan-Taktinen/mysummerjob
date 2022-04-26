#include "core/vulkan/device.h"

#include "logs/log.h"
#include "core/vulkan/utils.h"

#include <cassert>
#include <fstream>
#include <set>
#include <sstream>

namespace core::vk
{

// ----------------------------------------------------------------------------
//
//

Device::Device(
        VkPhysicalDevice gpu, const std::shared_ptr<GLFWwindow>& window) :
    m_Log(logs::Log::create("Vulkan Device"))
{
    assert(gpu);
    assert(window);
    m_Window = window;
    m_PhysicalDevice = gpu;

    vkGetPhysicalDeviceProperties(gpu, &m_PhysicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(gpu, &m_PhysicalDeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(gpu, &m_PhysicalDeviceMemoryProperties);

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
            gpu, &queueFamilyPropertyCount, nullptr);
    assert(queueFamilyPropertyCount > 0);
    m_QueueFamilyProperties.resize(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
            gpu, &queueFamilyPropertyCount, m_QueueFamilyProperties.data());
}

// ----------------------------------------------------------------------------
//
//

Device::~Device()
{
    if(m_CommandPools.graphics)
    {
        vkDestroyCommandPool(m_LogicalDevice, m_CommandPools.graphics, nullptr);
    }
    if(m_CommandPools.compute)
    {
        vkDestroyCommandPool(m_LogicalDevice, m_CommandPools.compute, nullptr);
    }
    if(m_CommandPools.transfer)
    {
        vkDestroyCommandPool(m_LogicalDevice, m_CommandPools.transfer, nullptr);
    }

    vmaDestroyAllocator(m_Allocator);
    if(m_LogicalDevice)
    {
        vkDestroyDevice(m_LogicalDevice, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

void Device::createLogicalDevice(
        const VkInstance& instance,
        std::vector<const char*> requestedExtensions,
        VkQueueFlags requestedQueueTypes)
{
    const float queuePriority[] = {1.0f};
    std::set<uint32_t> uniqueIndices = {};

    if(requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
    {
        m_QueueFamilyIndices.graphics =
                getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        uniqueIndices.insert(m_QueueFamilyIndices.graphics);
        m_Log->info(
                "Graphics Queue Family index({})",
                m_QueueFamilyIndices.graphics);
    }

    if(requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
    {
        m_QueueFamilyIndices.compute =
                getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        uniqueIndices.insert(m_QueueFamilyIndices.compute);
        m_Log->info(
                "Compute Queue Family index({})", m_QueueFamilyIndices.compute);
    }
    else
    {
        m_QueueFamilyIndices.compute = m_QueueFamilyIndices.graphics;
    }

    if(requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
    {
        m_QueueFamilyIndices.transfer =
                getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
        uniqueIndices.insert(m_QueueFamilyIndices.transfer);
        m_Log->info(
                "Transfer Queue Family index({})",
                m_QueueFamilyIndices.transfer);
    }
    else
    {
        m_QueueFamilyIndices.transfer = m_QueueFamilyIndices.graphics;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
    for(uint32_t uniqueIndex : uniqueIndices)
    {
        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pNext = nullptr;
        queueInfo.flags = 0;
        queueInfo.queueFamilyIndex = uniqueIndex;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = queuePriority;
        queueCreateInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
    indexingFeatures.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexingFeatures.pNext = nullptr;
    indexingFeatures.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceFeatures2KHR requestedFeatures = {};
    requestedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    requestedFeatures.features.samplerAnisotropy = VK_TRUE;
    requestedFeatures.pNext = &indexingFeatures;

    requestedExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &indexingFeatures;
    createInfo.flags = 0;
    createInfo.queueCreateInfoCount =
            static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.enabledExtensionCount =
            static_cast<uint32_t>(requestedExtensions.size());
    createInfo.ppEnabledExtensionNames = requestedExtensions.data();
    createInfo.pEnabledFeatures = &requestedFeatures.features;

    VK_CHECK(vkCreateDevice(
            m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice));

    assert(m_PhysicalDevice);
    assert(m_LogicalDevice);
    // Create allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_PhysicalDevice;
    allocatorInfo.device = m_LogicalDevice;
    allocatorInfo.instance = instance;

    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_Allocator));

    // Create commandpools and queues, assuming graphics is always required
    m_CommandPools.graphics = createCommandPool(
            m_QueueFamilyIndices.graphics,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    if(requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
    {
        m_CommandPools.compute = createCommandPool(
                m_QueueFamilyIndices.compute,
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    }

    if(requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
    {
        m_CommandPools.transfer =
                createCommandPool(m_QueueFamilyIndices.transfer, 0);
    }

    vkGetDeviceQueue(
            m_LogicalDevice,
            m_QueueFamilyIndices.graphics,
            0,
            &m_Queue.graphics);

    vkGetDeviceQueue(
            m_LogicalDevice,
            m_QueueFamilyIndices.graphics,
            0,
            &m_Queue.present);

    vkGetDeviceQueue(
            m_LogicalDevice, m_QueueFamilyIndices.compute, 0, &m_Queue.compute);

    vkGetDeviceQueue(
            m_LogicalDevice,
            m_QueueFamilyIndices.transfer,
            0,
            &m_Queue.transfer);
}

// ----------------------------------------------------------------------------
//
//

uint32_t Device::getQueueFamilyIndex(VkQueueFlagBits queueFlags) const
{
    assert(m_QueueFamilyProperties.size() > 0);
    // Try finding compute queue that is separate from graphics queue
    if(queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        for(size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
        {
            if((m_QueueFamilyProperties[i].queueFlags & queueFlags)
               && (m_QueueFamilyProperties[i].queueFlags
                   & VK_QUEUE_GRAPHICS_BIT)
                          == 0)
            {
                return static_cast<uint32_t>(i);
            }
        }
    }

    if(queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        for(size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
        {
            if((m_QueueFamilyProperties[i].queueFlags & queueFlags)
               && (m_QueueFamilyProperties[i].queueFlags
                   & VK_QUEUE_GRAPHICS_BIT)
                          == 0)
            {
                return static_cast<uint32_t>(i);
            }
        }
    }

    for(size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
    {
        if(m_QueueFamilyProperties[i].queueFlags & queueFlags)
        {
            return static_cast<uint32_t>(i);
        }
    }

    assert(false);
    return 0;
}

// ----------------------------------------------------------------------------
//
//

VkCommandBuffer Device::createCommandBuffer(
        VkCommandBufferLevel level, VkQueueFlags queueType, bool begin)
{
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.level = level;
    allocateInfo.commandBufferCount = 1;
    switch(queueType)
    {
    case VK_QUEUE_GRAPHICS_BIT:
        allocateInfo.commandPool = m_CommandPools.graphics;
        break;
    case VK_QUEUE_COMPUTE_BIT:
        allocateInfo.commandPool = m_CommandPools.compute;
        break;
    case VK_QUEUE_TRANSFER_BIT:
        allocateInfo.commandPool = m_CommandPools.transfer;
        break;
    default: m_Log->warn("Unsupported queue type"); assert(false);
    }

    VK_CHECK(vkAllocateCommandBuffers(
            m_LogicalDevice, &allocateInfo, &commandBuffer));

    if(begin)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    }

    return commandBuffer;
}

// ----------------------------------------------------------------------------
//
//

void Device::beginCommandBuffer(
        VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = nullptr;
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

// ----------------------------------------------------------------------------
//
//

void Device::flushCommandBuffer(
        VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = 0;

    VkFence fence;
    VK_CHECK(vkCreateFence(m_LogicalDevice, &fenceInfo, nullptr, &fence));

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));

    VK_CHECK(vkWaitForFences(m_LogicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));

    vkDestroyFence(m_LogicalDevice, fence, nullptr);

    if(free)
    {
        // TODO: cmdbuf may be alloceted from other than graphics pool
        vkFreeCommandBuffers(
                m_LogicalDevice, m_CommandPools.graphics, 1, &commandBuffer);
    }
}

void Device::createBuffer(
        VkBufferUsageFlags usage,
        VmaMemoryUsage vmaUsage,
        VkMemoryPropertyFlags properties,
        VkDeviceSize size,
        VkBuffer* buffer,
        VmaAllocation* bufferMemory,
        void* srcData)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = vmaUsage;
    allocInfo.preferredFlags = properties;

    VK_CHECK(vmaCreateBuffer(
            m_Allocator,
            &bufferInfo,
            &allocInfo,
            buffer,
            bufferMemory,
            nullptr));

    if(srcData)
    {
        void* dstData;
        vmaMapMemory(m_Allocator, *bufferMemory, &dstData);
        memcpy(dstData, srcData, size);
        vmaUnmapMemory(m_Allocator, *bufferMemory);
        if((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            vmaFlushAllocation(m_Allocator, *bufferMemory, 0, size);
        }
    }
}

// ----------------------------------------------------------------------------
//
//

void Device::createBufferOnGPU(
        VkBufferUsageFlags usage,
        VkDeviceSize size,
        VkBuffer* buffer,
        VmaAllocation* bufferMemory,
        void* data)
{
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;

    createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            size,
            &stagingBuffer,
            &stagingBufferMemory,
            data);

    createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            size,
            buffer,
            bufferMemory);

    copyBuffer(stagingBuffer, *buffer, size);
    vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingBufferMemory);
}

void Device::createImageOnGPU(
        VkImageUsageFlags usage,
        VkDeviceSize size,
        VkFormat format,
        VkImageLayout layout,
        VkExtent2D extent,
        VkImage* image,
        VmaAllocation* imageMemory,
        void* data)
{
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;

    createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            size,
            &stagingBuffer,
            &stagingBufferMemory,
            data);

    createImage(
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage,
            VMA_MEMORY_USAGE_GPU_ONLY,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            extent,
            image,
            imageMemory);

    VkCommandBuffer cmdBuf = createCommandBuffer();
    VkImageMemoryBarrier imageBarrier = {};
    {
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.pNext = nullptr;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = *image;
        imageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(
                cmdBuf,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageBarrier);
    }

    {
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {extent.width, extent.height, 1};

        vkCmdCopyBufferToImage(
                cmdBuf,
                stagingBuffer,
                *image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &copyRegion);
    }
    {

        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = layout;
        vkCmdPipelineBarrier(
                cmdBuf,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageBarrier);
    }

    flushCommandBuffer(cmdBuf, m_Queue.graphics, true);
    vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingBufferMemory);
}

// ----------------------------------------------------------------------------
//
//

void Device::copyBuffer(
        VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    VkCommandBuffer commandBuffer = createCommandBuffer(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, VK_QUEUE_TRANSFER_BIT);
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    flushCommandBuffer(commandBuffer, m_Queue.transfer, false);
    vkFreeCommandBuffers(
            m_LogicalDevice, m_CommandPools.transfer, 1, &commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void Device::copyBufferToImage(
        VkBuffer srcBuffer,
        VkImage dstImage,
        VkExtent2D extent,
        VkImageLayout layout,
        VkDeviceSize size)
{

    (void)layout;
    (void)size;

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    // copyRegion.bufferRowLength;
    // copyRegion.bufferImageHeight;
    copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = {extent.width, extent.height, 1};

    VkCommandBuffer commandBuffer = createCommandBuffer(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, VK_QUEUE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(
            commandBuffer,
            srcBuffer,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

    flushCommandBuffer(commandBuffer, m_Queue.transfer, false);
    vkFreeCommandBuffers(
            m_LogicalDevice, m_CommandPools.transfer, 1, &commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

VkCommandPool Device::createCommandPool(
        uint32_t queueFamilyIndex, VkCommandPoolCreateFlags poolFlags) const
{
    VkCommandPool commandPool;
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = poolFlags;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    VK_CHECK(vkCreateCommandPool(
            m_LogicalDevice, &createInfo, nullptr, &commandPool));
    return commandPool;
}

// ----------------------------------------------------------------------------
//
//

void Device::createImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect,
        VkImageView* view)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange = {aspect, 0, 1, 0, 1};

    VK_CHECK(vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, view));
}

// ----------------------------------------------------------------------------
//
//

void Device::createImage(
        VkImageUsageFlags usage,
        VmaMemoryUsage vmaMemoryUsage,
        VkFormat format,
        VkImageTiling tiling,
        VkExtent2D extent,
        VkImage* image,
        VmaAllocation* memory,
        VmaAllocationCreateFlags vmaFlags,
        VkImageCreateFlags imageFlags,
        uint32_t mipLevels,
        uint32_t arrayLayers)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.flags = imageFlags;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // imageInfo.queueFamilyIndexCount;
    // imageInfo.pQueueFamilyIndices;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.flags = vmaFlags;
    allocInfo.usage = vmaMemoryUsage;

    VK_CHECK(vmaCreateImage(
            m_Allocator, &imageInfo, &allocInfo, image, memory, nullptr));
}

// ----------------------------------------------------------------------------
//
//

void Device::createSampler(VkSampler* sampler)
{
    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.mipLodBias = 0.0f;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 16.0f;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_NEVER;
    // createInfo.minLod;
    // createInfo.maxLod;
    // createInfo.borderColor;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    VK_CHECK(vkCreateSampler(m_LogicalDevice, &createInfo, nullptr, sampler));
}

// ----------------------------------------------------------------------------
//
//

VkPipelineShaderStageCreateInfo Device::loadShaderFromFile(
        const std::string& path, VkShaderStageFlagBits stage)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if(!file)
    {
        m_Log->warn("Unable to open shader file {}", path);
        assert(false);
        throw std::runtime_error("");
    }
    m_Log->info("Load shader {}", path);
    std::ostringstream oss;
    oss << file.rdbuf();
    const auto code = oss.str();

    VkPipelineShaderStageCreateInfo shaderStageInfo = {};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext = nullptr;
    shaderStageInfo.stage = stage;
    shaderStageInfo.pName = "main";
    shaderStageInfo.pSpecializationInfo = nullptr;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.c_str());
    VK_CHECK(vkCreateShaderModule(
            m_LogicalDevice, &createInfo, nullptr, &shaderStageInfo.module));
    return shaderStageInfo;
}

// ----------------------------------------------------------------------------
//
//

uint32_t Device::findMemoryType(
        uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if((typeFilter & (1 << i))
           && (memProperties.memoryTypes[i].propertyFlags & properties)
                      == properties)
        {
            return i;
        }
    }

    m_Log->warn("failed to find suitable memory type!");
    assert(false);
    throw std::runtime_error("");
}

const std::shared_ptr<GLFWwindow>& Device::getGLFWwindow() const
{
    return m_Window;
}
} // namespace core::vk
