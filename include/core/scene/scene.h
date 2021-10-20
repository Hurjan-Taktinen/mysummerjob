#pragma once

#include "core/scene/camera.h"
#include "core/model/model.h"
#include "core/vulkan/device.h"

#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace core::scene
{
class Scene
{
public:
    Scene(entt::registry& registry, TrackBall* cam);
    void loadModels(vk::Device* device);
    void addModel(vk::Device* device, const std::string& file);
    const auto& getDrawList() const { return m_Models; }
    auto getDescriptorWrites() const { return true; }

    void clear()
    {
        m_Models.clear();
        m_Registry.clear();
    }

    auto getbufferinfos() const
    {
        std::vector<VkDescriptorBufferInfo> bufferInfos = {};
        for(const auto& model : m_Models)
        {
            bufferInfos.push_back(model.getBufferInfo());
        }
        return bufferInfos;
    }

    auto getImageInfos() const
    {
        std::vector<VkDescriptorImageInfo> imageInfos = {};
        for(const auto& model : m_Models)
        {
            const auto& im = model.getImageInfos();
            imageInfos.insert(imageInfos.end(), im.begin(), im.end());
        }
        return imageInfos;
    }

    const auto* getCamera() const { return m_Camera;}

private:
    entt::registry& m_Registry;
    TrackBall* m_Camera = nullptr;
    std::vector<model::Model> m_Models;
};
} // namespace scene
