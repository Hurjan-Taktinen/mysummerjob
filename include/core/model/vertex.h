#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vulkan/vulkan.h>

#include <utility>
#include <array>
#include <vector>

namespace core::model
{
struct VertexPC final
{
    VertexPC(const glm::vec3& pp, const glm::vec3& cc) : p(pp), c(cc) {}

    glm::vec3 p;
    glm::vec3 c;

    static auto getBindingDescription()
    {
        VkVertexInputBindingDescription desc;
        desc.binding = 0;
        desc.stride = sizeof(VertexPC);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static auto getAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 2> desc = {};
        desc[0].location = 0;
        desc[0].binding = 0;
        desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[0].offset = offsetof(VertexPC, p);

        desc[1].location = 1;
        desc[1].binding = 0;
        desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[1].offset = offsetof(VertexPC, c);

        return desc;
    }
};

struct VertexPT final
{
    VertexPT(const glm::vec3& pp, const glm::vec2& tt) : p(pp), t(tt) {}

    glm::vec3 p;
    glm::vec2 t;

    static auto getBindingDescription()
    {
        VkVertexInputBindingDescription desc;
        desc.binding = 0;
        desc.stride = sizeof(VertexPT);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static auto getAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 2> desc = {};
        desc[0].location = 0;
        desc[0].binding = 0;
        desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[0].offset = offsetof(VertexPT, p);

        desc[1].location = 1;
        desc[1].binding = 0;
        desc[1].format = VK_FORMAT_R32G32_SFLOAT;
        desc[1].offset = offsetof(VertexPT, t);

        return desc;
    }
};

struct VertexPNTC final
{
    VertexPNTC() = default;
    VertexPNTC(
            const glm::vec3& pp,
            const glm::vec3& nn,
            const glm::vec2& tt,
            const glm::vec3& cc) :
        p(pp), n(nn), t(tt), c(cc)
    {
    }

    glm::vec3 p;
    glm::vec3 n;
    glm::vec2 t;
    glm::vec3 c;
    int materialId = 0;

    static auto getBindingDescription()
    {
        VkVertexInputBindingDescription desc;
        desc.binding = 0;
        desc.stride = sizeof(VertexPNTC);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }
    static auto getAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 5> desc = {};
        desc[0].location = 0;
        desc[0].binding = 0;
        desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[0].offset = offsetof(VertexPNTC, p);

        desc[1].location = 1;
        desc[1].binding = 0;
        desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[1].offset = offsetof(VertexPNTC, n);

        desc[2].location = 2;
        desc[2].binding = 0;
        desc[2].format = VK_FORMAT_R32G32_SFLOAT;
        desc[2].offset = offsetof(VertexPNTC, t);

        desc[3].location = 3;
        desc[3].binding = 0;
        desc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[3].offset = offsetof(VertexPNTC, c);

        desc[4].location = 4;
        desc[4].binding = 0;
        desc[4].format = VK_FORMAT_R32_SINT;
        desc[4].offset = offsetof(VertexPNTC, materialId);

        return desc;
    }
};

struct VertexImGui final
{
    // Interface only for binding/attribute descriptions
    //
    static auto getBindingDescription()
    {
        VkVertexInputBindingDescription desc;
        desc.binding = 0;
        desc.stride = 20;
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static auto getAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 3> desc = {};
        desc[0].location = 0;
        desc[0].binding = 0;
        desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        desc[0].offset = 0;

        desc[1].location = 1;
        desc[1].binding = 0;
        desc[1].format = VK_FORMAT_R32G32_SFLOAT;
        desc[1].offset = 2 * sizeof(float);

        desc[2].location = 2;
        desc[2].binding = 0;
        desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        desc[2].offset = 4 * sizeof(float);

        return desc;
    }
};

} // namespace core::model
