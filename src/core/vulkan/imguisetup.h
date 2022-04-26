#pragma once

#include "core/vulkan/device.h"
#include "core/vulkan/utils.h"
#include "core/texture/texture.h"
#include "logs/log.h"

#include "glm/vec2.hpp"
#include "imgui/imgui.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <array>

namespace core::vk
{

class ImGuiSetup
{
public:
    ImGuiSetup(Device* device, VkRenderPass renderpass);
    ~ImGuiSetup();

    void update();
    void draw(VkCommandBuffer cmdBuf);

    struct PushConstantBlock
    {
        glm::vec2 scale;
        glm::vec2 translate;
    } pushConstants;

private:
    void setDarkTheme();

    logs::Logger m_Log;
    Device* m_Device;

    int32_t vertexCount = 0;
    int32_t indexCount = 0;

    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;

    VmaAllocation m_VertexMemory = VK_NULL_HANDLE;
    VmaAllocation m_IndexMemory = VK_NULL_HANDLE;

    texture::Texture2d m_FontTexture;

    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet;
};

} // namespace core::vk
