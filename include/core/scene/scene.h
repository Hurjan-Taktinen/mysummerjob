#pragma once

#include "core/scene/camera.h"
#include "core/model/model.h"
#include "core/vulkan/device.h"

#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "core/scene/components.h"

namespace core::scene
{
class Scene
{
public:
    Scene(entt::registry& registry, TrackBall* cam);
    void loadModels(vk::Device* device);
    void addModel(vk::Device* device, const std::string& file);
    [[nodiscard]] const auto& getDrawList() const { return m_Models; }
    auto getDescriptorWrites() const { return true; }

    void clear()
    {
        m_Models.clear();
        m_Registry.clear();
    }

    [[nodiscard]] inline auto getbufferinfos() const
    {
        std::vector<VkDescriptorBufferInfo> bufferInfos = {};
        for(const auto& model : m_Models)
        {
            bufferInfos.push_back(model.getBufferInfo());
        }
        return bufferInfos;
    }

    [[nodiscard]] inline auto getImageInfos() const
    {
        std::vector<VkDescriptorImageInfo> imageInfos = {};
        for(const auto& model : m_Models)
        {
            const auto& im = model.getImageInfos();
            imageInfos.insert(imageInfos.end(), im.begin(), im.end());
        }
        return imageInfos;
    }

    [[nodiscard]] const auto* getCamera() const { return m_Camera; }

    inline void updatePositions(float time)
    {
        auto view = m_Registry.view<scene::component::Position>();
        float i = 0;
        for(auto entity : view)
        {
            const float rad = glm::radians((i + time) * 360.0f / 12);
            auto& pos = view.get<scene::component::Position>(entity);
            pos.pos =
                    glm::vec4(glm::vec3(sin(rad), cos(rad), 0.0f) * 5.0f, 1.0f);
            i += 1.0f;
        }
    }

private:
    entt::registry& m_Registry;
    TrackBall* m_Camera = nullptr;
    std::vector<model::Model> m_Models;
};
} // namespace core::scene
