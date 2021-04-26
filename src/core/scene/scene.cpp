#include "core/scene/scene.h"
#include "core/scene/components.h"

#include "logs/log.h"

namespace core::scene
{

Scene::Scene(entt::registry& registry, TrackBall* cam) :
    m_Registry(registry), m_Camera(cam)
{
}

void Scene::loadModels(vk::Device* device)
{
    {
        m_Models.emplace_back(device, m_Registry);
        m_Models.back().load("data/models/hurja.obj");
    }
}

} // namespace core::scene
