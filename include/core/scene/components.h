#pragma once

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "vulkan/vulkan.h"

#include <vector>

namespace core::scene::component
{

struct Position
{
    glm::vec3 pos;
    constexpr operator glm::vec3() const { return pos; }
};

struct Transform
{
    glm::mat4 transform;
    constexpr operator glm::mat4() const { return transform; }
};

struct ImageInfo
{
    std::vector<VkDescriptorImageInfo> infos;
    // constexpr operator VkDescriptorImageInfo() const { return info; }
};

struct BufferInfo
{
    VkDescriptorBufferInfo infos;
    // constexpr operator VkDescriptorBufferInfo() const { return info; }
};

struct VertexInfo
{
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
};

struct RenderInfo
{
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    size_t numIndices = 0;
    // VkDescriptorSet descriptor = VK_NULL_HANDLE;
    VkDescriptorBufferInfo buffeInfo;
    std::vector<VkDescriptorImageInfo> imageInfos;
};

} // namespace core::scene::component
