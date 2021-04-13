#pragma once

#include "core/scene/camera.h"
#include "core/model/model.h"
#include "core/vulkan/device.h"

#include "entt/entt.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace core::scene
{
class Scene
{
public:
    Scene(TrackBall* camera);
    void loadModels(vk::Device* device);
    // void draw(VkCommandBuffer cmdBuf) const;
    const auto& getDrawList() const { return m_Models; }
    auto getDescriptorWrites() const { return true; }
    const auto* getCamera() const { return m_Camera; }

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

private:
    entt::registry m_Registry;
    TrackBall* m_Camera = nullptr;
    std::vector<model::Model> m_Models;
};
} // namespace scene
