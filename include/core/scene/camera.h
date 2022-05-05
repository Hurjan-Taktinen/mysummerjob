#pragma once

#include "logs/log.h"

#include "event/sub.h"
#include "event/updateevents.h"

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/constants.hpp"

#include <numbers>

namespace core::scene
{

class TrackBall final : public std::enable_shared_from_this<TrackBall>
{
public:
    TrackBall(entt::dispatcher& disp) :
        m_Log(logs::Log::create("Camera")), m_conn(disp, this)
    {
    }

    struct
    {
        glm::mat4 view;
        glm::mat4 proj;
    } matrices;

    void setProjection(uint32_t width, uint32_t height);
    void update(float dt);

    void updateRotation(float dtheta, float dphi);
    void updateZoom(float delta);
    void pan(float dx, float dy);

    [[nodiscard]] const glm::vec3& getPosition() const { return m_Position; }

private:
    void updateViewMatrix();
    [[nodiscard]] glm::vec3 toCartesian() const;

    logs::Logger m_Log;
    event::Subs<TrackBall> m_conn;

    float m_Radius = 20.0f;
    glm::vec3 m_Target = glm::vec3(0);
    glm::vec3 m_Position = glm::vec3(0, 0, m_Radius);

    float m_Theta = std::numbers::pi;
    float m_Phi = std::numbers::pi * 0.5f;
    glm::vec3 m_Up = glm::vec3(0, 1, 0);

    float m_Fov = 45.0f;
    float m_zNear = 0.1f;
    float m_zFar = 1000.0f;

    float m_MouseSensitivity = 0.004f;
    float m_PanSensitivity = 0.01f;
    float m_ScrollSensitivity = 1.0f;

    // const float m_FovMin = 5.0f;
    // const float m_FovMax = 120.0f;
};

} // namespace core::scene
