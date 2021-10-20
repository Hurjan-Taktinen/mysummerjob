#include "core/scene/scene.h"
#include "core/scene/components.h"

#include "logs/log.h"

namespace core::scene
{

Scene::Scene(entt::registry& registry, TrackBall* cam) :
    m_Registry(registry), m_Camera(cam)
{
    m_Models.reserve(100);
}

void Scene::loadModels(vk::Device* device)
{
    auto& it = m_Models.emplace_back(device, m_Registry);
    it.load("data/models/hurja.obj");
}

void Scene::addModel(vk::Device* device, const std::string& file)
{
    auto& it = m_Models.emplace_back(device, m_Registry);
    it.load(file);
}

} // namespace core::scene
