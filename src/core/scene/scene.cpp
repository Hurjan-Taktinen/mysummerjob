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
    const int numHurjas = 12;
    for(int i = 0; i < numHurjas; ++i)
    {
        auto& it = m_Models.emplace_back(device, m_Registry);
        it.load("data/models/hurja.obj");
    }

    int i = 0;
    auto view = m_Registry.view<scene::component::Position>();
    for(auto entity : view)
    {
        const float rad =
                glm::radians(i * 360.0f / static_cast<uint32_t>(numHurjas));
        auto& pos = view.get<scene::component::Position>(entity);
        pos.pos = glm::vec4(glm::vec3(sin(rad), cos(rad), 0.0f) * 5.0f, 1.0f);
        i += 1;
    }
}

void Scene::addModel(vk::Device* device, const std::string& file)
{
    auto& it = m_Models.emplace_back(device, m_Registry);
    it.load(file);
}

} // namespace core::scene
