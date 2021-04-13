#include "core/scene/scene.h"

#include "logs/log.h"

namespace core::scene
{

// struct position
// {
// float x;
// float y;
// };

// struct velocity
// {
// float dx;
// float dy;
// };

// void update(entt::registry& registry)
// {
// auto view = registry.view<const position, velocity>();

// // use a callback
// view.each([](const auto& pos, auto& vel) {
// INFO("ENTITY pos({} {})", pos.x, pos.y);
// });

// // use an extended callback
// view.each([](const auto entity, const auto& pos, auto& vel) { [> ... <] });

// // use a range-for
// for(auto [entity, pos, vel] : view.each())
// {
// // ...
// }

// // use forward iterators and get only the components of interest
// for(auto entity : view)
// {
// auto& vel = view.get<velocity>(entity);
// INFO("ENTITY vel({}, {})", vel.dx, vel.dy);
// // ...
// }
// }

Scene::Scene(TrackBall* camera) : m_Camera(camera)
{
    // const auto cameraEntity = m_Registry.create();

    // entt::registry registry;

    // for(auto i = 0u; i < 10u; ++i)
    // {
    // const auto entity = registry.create();
    // registry.emplace<position>(entity, i * 1.f, i * 1.f);
    // if(i % 2 == 0)
    // {
    // registry.emplace<velocity>(entity, i * .1f, i * .1f);
    // }
    // }

    // update(registry);
}

void Scene::loadModels(vk::Device* device)
{
    // m_Model->load("data/models/hurja.obj");

    // Iterate over model files
    for(std::size_t i = 0; i < 1; ++i)
    {
        m_Models.emplace_back(device);
        m_Models.back().load("data/models/hurja.obj");
    }
}

} // namespace scene
