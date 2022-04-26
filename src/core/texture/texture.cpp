#include "core/texture/texture.h"

#include "core/texture/gli.h"
#include "stb/stb_image.h"
#include "core/vulkan/utils.h"
#include "logs/log.h"
#include "utils/stringutils.h"

#include <filesystem>
#include <algorithm>
#include <string>

namespace core::texture
{
void Texture2d::loadFromFile(
        vk::Device* device,
        const std::string& file,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        VkImageLayout imageLayout)
{
    std::string extension = utils::getExtension(file);

    assert(this->device);
    this->device = device;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingMemory = VK_NULL_HANDLE;

    std::vector<VkExtent2D> mipExtents;
    std::vector<uint32_t> mipSizes;

    m_Log->info("Loading texture {}", file);

    if(extension == ".ktx")
    {
        m_Log->info("Deduced texture type KTX");

        // If already initialized, do not do it again
        gli::texture2d tex2d(gli::load(file.c_str()));
        assert(!tex2d.empty());

        width = static_cast<uint32_t>(tex2d.extent().x);
        height = static_cast<uint32_t>(tex2d.extent().y);
        mipLevels = static_cast<uint32_t>(tex2d.levels());
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(tex2d.size());
        layout = imageLayout;

        for(size_t i = 0; i < mipLevels; ++i)
        {
            mipExtents.push_back(
                    {static_cast<uint32_t>(tex2d[i].extent().x),
                     static_cast<uint32_t>(tex2d[i].extent().y)});
            mipSizes.push_back(tex2d[i].size());
        }

        device->createBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                imageSize,
                &stagingBuffer,
                &stagingMemory,
                tex2d.data());
    }
    else if(extension == ".png" || extension == ".jpg")
    {
        m_Log->info("Deduced texture type PNG/JPG");
        int texChannels = 0;
        int texWidth = 0;
        int texHeight = 0;
        stbi_uc* data = stbi_load(
                file.c_str(),
                &texWidth,
                &texHeight,
                &texChannels,
                STBI_rgb_alpha);

        width = static_cast<uint32_t>(texWidth);
        height = static_cast<uint32_t>(texHeight);
        mipLevels = 1;
        layout = imageLayout;

        VkDeviceSize imageSize =
                static_cast<VkDeviceSize>(texWidth * texHeight * 4);

        for(size_t i = 0; i < 1; ++i)
        {
            mipExtents.push_back(
                    {static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height)});
            mipSizes.push_back(width * height);
        }

        device->createBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                imageSize,
                &stagingBuffer,
                &stagingMemory,
                data);

        stbi_image_free(data);
    }
    else
    {
        assert(false);
    }

    {
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = imageUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.flags = 0;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateImage(
                device->getAllocator(),
                &imageCreateInfo,
                &allocInfo,
                &image,
                &memory,
                nullptr));
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkCommandBuffer cmdBuf = device->createCommandBuffer(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, VK_QUEUE_TRANSFER_BIT, true);
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
        imageBarrier.image = image;
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = mipLevels;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = 1;
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

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    for(uint32_t i = 0; i < mipLevels; ++i)
    {
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = offset;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = i;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent.width = mipExtents[i].width;
        copyRegion.imageExtent.height = mipExtents[i].height;
        copyRegion.imageExtent.depth = 1;

        offset += mipSizes[i];
        bufferCopyRegions.push_back(copyRegion);
    }

    vkCmdCopyBufferToImage(
            cmdBuf,
            stagingBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(bufferCopyRegions.size()),
            bufferCopyRegions.data());

    {

        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = imageLayout;
        vkCmdPipelineBarrier(
                cmdBuf,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageBarrier);
    }

    // Submit & cleanup
    device->flushCommandBuffer(cmdBuf, device->getTransferQueue(), false);
    vkFreeCommandBuffers(
            device->getLogicalDevice(),
            device->getTransferCommandPool(),
            1,
            &cmdBuf);
    vmaDestroyBuffer(device->getAllocator(), stagingBuffer, stagingMemory);

    {
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.pNext = nullptr;
        samplerCreateInfo.flags = 0;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = 16.0f;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = static_cast<float>(mipLevels);
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        VK_CHECK(vkCreateSampler(
                device->getLogicalDevice(),
                &samplerCreateInfo,
                nullptr,
                &sampler));
    }

    {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        viewCreateInfo.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A};
        viewCreateInfo.subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        viewCreateInfo.subresourceRange.levelCount = mipLevels;
        viewCreateInfo.image = image;
        VK_CHECK(vkCreateImageView(
                device->getLogicalDevice(), &viewCreateInfo, nullptr, &view));
    }

    updateDescriptor();
}

void Texture2d::loadFromBuffer(
        vk::Device* device,
        void* buffer,
        VkDeviceSize bufferSize,
        VkFormat format,
        uint32_t width,
        uint32_t height,
        VkFilter filter,
        VkImageUsageFlags imageUsage,
        VkImageLayout imageLayout)
{
    assert(buffer);
    this->device = device;
    this->width = width;
    this->height = height;
    this->mipLevels = 1;
    this->layout = imageLayout;

    VkBuffer stagingBuffer;
    VmaAllocation stagingMemory;

    device->createBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            bufferSize,
            &stagingBuffer,
            &stagingMemory,
            buffer);

    VkExtent2D extent{width, height};

    device->createImage(
            imageUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            extent,
            &image,
            &memory);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkCommandBuffer cmdBuf = device->createCommandBuffer(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, VK_QUEUE_TRANSFER_BIT, true);
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
        imageBarrier.image = image;
        imageBarrier.subresourceRange = subresourceRange;
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

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent.width = width;
    copyRegion.imageExtent.height = height;
    copyRegion.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
            cmdBuf,
            stagingBuffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

    {

        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = imageLayout;
        vkCmdPipelineBarrier(
                cmdBuf,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageBarrier);
    }

    // Submit & cleanup
    device->flushCommandBuffer(cmdBuf, device->getTransferQueue(), false);
    vkFreeCommandBuffers(
            device->getLogicalDevice(),
            device->getTransferCommandPool(),
            1,
            &cmdBuf);

    vmaDestroyBuffer(device->getAllocator(), stagingBuffer, stagingMemory);

    {
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.pNext = nullptr;
        samplerCreateInfo.flags = 0;
        samplerCreateInfo.magFilter = filter;
        samplerCreateInfo.minFilter = filter;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = 16.0f;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        VK_CHECK(vkCreateSampler(
                device->getLogicalDevice(),
                &samplerCreateInfo,
                nullptr,
                &sampler));
    }

    {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        viewCreateInfo.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A};
        viewCreateInfo.subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.image = image;
        VK_CHECK(vkCreateImageView(
                device->getLogicalDevice(), &viewCreateInfo, nullptr, &view));
    }

    descriptor = VkDescriptorImageInfo{sampler, view, layout};
}

} // namespace core::texture
