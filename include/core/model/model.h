#pragma once

#include "core/vulkan/device.h"
#include "core/texture/texture.h"
#include "core/model/vertex.h"
#include "logs/log.h"
#include "glm/vec3.hpp"
#include "entt/entity/registry.hpp"

#include <memory>
#include <vector>
#include <string_view>

namespace core::model
{

struct MaterialUbo
{
    glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse = glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 transmittance = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 emission = glm::vec3(0.0f, 0.0f, 0.0f);
    float shininess = 0.0f;
    float metallic = 0.0f;
    float ior = 1.0f;
    float dissolve = 1.0f;
    int illum = 0;
    int diffuseTextureID = -1;
    int specularTextureID = -1;
    int normalTextureID = -1;
    float pad = 0.0f;
};

enum struct TextureType
{
    Diffuse,
    Alpha,
    Displacement,
    Normal,
    Environment,
    Specular
};

class Model
{
public:
    explicit Model(core::vk::Device* device, entt::registry& reg) :
        m_Device(device), m_Registry(reg)
    {
        if(!m_Log)
        {
            m_Log = logs::Log::create("Model");
        }
    }

    ~Model();

    void load(const std::string& path);

    auto getMaterialBuffer() const { return m_MaterialBuffer; }
    const auto& getVertices() const { return m_Vertices; }
    const auto& getIndices() const { return m_Indices; }
    const auto* VertexBuffer() const { return &m_VertexBuffer; }
    const auto& IndexBuffer() const { return m_IndexBuffer; }

    auto getImageInfos() const { return m_Infos; }
    auto getBufferInfo() const
    {
        VkDescriptorBufferInfo info = {};
        info.buffer = m_MaterialBuffer;
        info.offset = 0;
        info.range = VK_WHOLE_SIZE;

        return info;
    }

private:
    struct PushConstantBlock
    {
        glm::vec4 position = glm::vec4(0.0f);
    } m_PushConstants;

    inline static logs::Logger m_Log;
    vk::Device* m_Device;
    entt::registry& m_Registry;

    std::vector<VertexPNTC> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<MaterialUbo> m_Materials;
    std::vector<std::string> m_TexturePaths;
    std::vector<texture::Texture2d> m_Textures;
    std::vector<VkDescriptorImageInfo> m_Infos;

    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_VertexMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_IndexMemory = VK_NULL_HANDLE;
    VkBuffer m_MaterialBuffer = VK_NULL_HANDLE;
    VmaAllocation m_MaterialMemory = VK_NULL_HANDLE;

};

} // namespace core::model
