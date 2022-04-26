#include "core/model/model.h"

#include "core/vulkan/descriptorgen.h"
#include "logs/log.h"
#include "tinyobj/tiny_obj_loader.h"
#include "core/scene/components.h"
#include "utils/stringutils.h"

#include <glm/glm.hpp>

#include <cassert>

namespace core::model
{

Model::~Model()
{
    if(m_VertexBuffer)
    {
        vmaDestroyBuffer(
                m_Device->getAllocator(), m_VertexBuffer, m_VertexMemory);
    }
    if(m_IndexBuffer)
    {
        vmaDestroyBuffer(
                m_Device->getAllocator(), m_IndexBuffer, m_IndexMemory);
    }
    if(m_MaterialBuffer)
    {
        vmaDestroyBuffer(
                m_Device->getAllocator(), m_MaterialBuffer, m_MaterialMemory);
    }
}

void Model::load(const std::string& path)
{
    tinyobj::ObjReader reader;
    if(!reader.ParseFromFile(path))
    {
        if(!reader.Error().empty())
        {
            m_Log->critical(
                    "Failed to load {}. TinyObjReader: {}",
                    path,
                    reader.Error());
        }

        return;
    }

    if(!reader.Warning().empty())
    {
        m_Log->warn("TinyObjReader: {}", reader.Warning());
    }

    const auto directory = utils::getDirectory(path);

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    m_Log->info("Model {} loaded", path);
    m_Log->info("      {} shapes", shapes.size());
    m_Log->info("      {} materials", materials.size());

    for(const auto& mat : materials)
    {
        MaterialUbo m;

        m.ambient = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
        m.diffuse = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        m.specular =
                glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        m.emission =
                glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
        m.transmittance = glm::vec3(
                mat.transmittance[0],
                mat.transmittance[1],
                mat.transmittance[2]);
        m.dissolve = mat.dissolve;
        m.ior = mat.ior;
        m.illum = mat.illum;

        if(mat.roughness == 0.0f)
        {
            m.shininess = 0.0f;
        }
        else
        {
            m.shininess = 1.0f - mat.roughness;
        }

        { // Diffuse
            if(!mat.diffuse_texname.empty())
            {
                auto ret = directory + "/" + mat.diffuse_texname;
                m_Log->info("DIFFUSE TEXTURE NAME {}", ret);
                m_TexturePaths.push_back(ret);
                m.diffuseTextureID =
                        static_cast<uint32_t>(m_TexturePaths.size() - 1);
            }
        }
        { // Specular
            if(!mat.specular_texname.empty())
            {
                auto ret = directory + "/" + mat.specular_texname;
                m_Log->info("SPECULAR TEXTURE NAME {}", ret);
                m_TexturePaths.push_back(ret);
                m.specularTextureID =
                        static_cast<uint32_t>(m_TexturePaths.size() - 1);
            }
        }
        { // Normal
            if(!mat.normal_texname.empty())
            {
                auto ret = directory + "/" + mat.normal_texname;
                m_Log->info("NORMAL TEXTURE NAME {}", ret);
                m_TexturePaths.push_back(ret);
                m.normalTextureID =
                        static_cast<uint32_t>(m_TexturePaths.size() - 1);
            }
        }
        m_Materials.push_back(m);
    }

    if(m_Materials.empty())
    {
        m_Materials.emplace_back(MaterialUbo());
    }

    for(const auto& shape : shapes)
    {
        uint32_t faceId = 0;
        int indexCount = 0;

        for(const auto& index : shape.mesh.indices)
        {
            VertexPNTC vertex = {};
            vertex.p.x = attrib.vertices[3 * index.vertex_index + 0];
            vertex.p.y = attrib.vertices[3 * index.vertex_index + 1];
            vertex.p.z = attrib.vertices[3 * index.vertex_index + 2];

            if(!attrib.normals.empty() && index.normal_index >= 0)
            {
                vertex.n.x = attrib.normals[3 * index.normal_index + 0];
                vertex.n.y = attrib.normals[3 * index.normal_index + 1];
                vertex.n.z = attrib.normals[3 * index.normal_index + 2];
            }

            if(!attrib.texcoords.empty() && index.texcoord_index >= 0)
            {
                vertex.t.x = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.t.y =
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
            }

            if(!attrib.colors.empty())
            {
                vertex.c.x = attrib.colors[3 * index.vertex_index + 0];
                vertex.c.y = attrib.colors[3 * index.vertex_index + 1];
                vertex.c.z = attrib.colors[3 * index.vertex_index + 2];
            }

            vertex.materialId = shape.mesh.material_ids[faceId];
            if(vertex.materialId < 0
               || vertex.materialId >= static_cast<int>(m_Materials.size()))
            {
                vertex.materialId = 0;
            }

            indexCount += 1;
            if(indexCount >= 3)
            {
                faceId += 1;
                indexCount = 0;
            }

            m_Vertices.push_back(std::move(vertex));
            m_Indices.push_back(static_cast<uint32_t>(m_Indices.size()));
        }
    }

    if(attrib.normals.empty())
    {
        m_Log->info("Normals not included with model, generating...");

        for(size_t i = 0; i < m_Indices.size(); ++i)
        {
            auto& v0 = m_Vertices[m_Indices[i + 0]];
            auto& v1 = m_Vertices[m_Indices[i + 1]];
            auto& v2 = m_Vertices[m_Indices[i + 2]];

            auto normal = glm::normalize(glm::cross(v1.p - v0.p, v2.p - v0.p));
            v0.n = normal;
            v1.n = normal;
            v2.n = normal;
        }

        m_Log->info("{} Normals generated", m_Vertices.size());
    }

    { // Vertices
        VkDeviceSize size = sizeof(VertexPNTC) * m_Vertices.size();
        m_Device->createBufferOnGPU(
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                size,
                &m_VertexBuffer,
                &m_VertexMemory,
                m_Vertices.data());
    }

    { // Indices
        VkDeviceSize size = sizeof(uint32_t) * m_Indices.size();
        m_Device->createBufferOnGPU(
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                size,
                &m_IndexBuffer,
                &m_IndexMemory,
                m_Indices.data());
    }

    { // Materials
        VkDeviceSize size = sizeof(MaterialUbo) * m_Materials.size();
        m_Device->createBufferOnGPU(
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                size,
                &m_MaterialBuffer,
                &m_MaterialMemory,
                m_Materials.data());
    }

    m_Textures.resize(m_TexturePaths.size());
    for(size_t i = 0; i < m_TexturePaths.size(); ++i)
    {
        const auto& file = m_TexturePaths[i];
        auto& texture = m_Textures[i];
        texture.loadFromFile(m_Device, file, VK_FORMAT_R8G8B8A8_UNORM);
    }

    std::vector<VkDescriptorImageInfo> infos;
    for(const auto& tex : m_Textures)
    {
        VkDescriptorImageInfo info = {};
        info.sampler = tex.sampler;
        info.imageView = tex.view;
        info.imageLayout = tex.layout;
        infos.push_back(info);
    }

    scene::component::VertexInfo vertexInfo{
            .numIndices = m_Indices.size(),
            .vertexBuffer = m_VertexBuffer,
            .indexBuffer = m_IndexBuffer,
    };

    scene::component::RenderInfo renderInfo{
            .buffeInfo = {m_MaterialBuffer, 0, VK_WHOLE_SIZE},
            .imageInfos = std::move(infos)};

    const auto ent = m_Registry.create();
    auto transform = glm::mat4{1.0f};
    auto position = glm::vec3{0.0f, 0.0f, 0.0f};

    glm::translate(transform, position);

    m_Registry.emplace<scene::component::Position>(ent, position);
    m_Registry.emplace<scene::component::Transform>(ent, transform);
    m_Registry.emplace<scene::component::VertexInfo>(ent, vertexInfo);
    m_Registry.emplace<scene::component::RenderInfo>(ent, renderInfo);
}

} // namespace core::model
