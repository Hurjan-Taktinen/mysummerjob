#include "core/scene/camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <iostream>

namespace core::scene
{

// ----------------------------------------------------------------------------
//
//

void TrackBall::setProjection(uint32_t width, uint32_t height)
{
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    matrices.proj =
            glm::perspective(glm::radians(m_Fov), aspect, m_zNear, m_zFar);
    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::updateRotation(float dtheta, float dphi)
{
    using namespace std::numbers;

    dtheta *= m_MouseSensitivity;
    dphi *= m_MouseSensitivity;

    if(m_Up.y > 0.0f)
    {
        m_Theta += dtheta;
    }
    else
    {
        m_Theta -= dtheta;
    }

    m_Phi += dphi;

    // Clamp phi between -2PI and 2PI
    if(m_Phi > 2.0f * pi)
    {
        m_Phi -= 2.0f * pi;
    }
    else if(m_Phi < -2.0f * pi)
    {
        m_Phi += 2.0f * pi;
    }

    if((0.0f < m_Phi && m_Phi < pi) || (-pi > m_Phi && m_Phi > -2.0f * pi))
    {
        m_Up.y = 1.0f;
    }
    else
    {
        m_Up.y = -1.0f;
    }

    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::updateZoom(float dradius)
{
    m_Radius -= dradius * m_ScrollSensitivity;
    if(m_Radius <= 0.00001f)
    {
        m_Radius = 0.01f;
    }

    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::pan(float dx, float dy)
{
    dx *= m_PanSensitivity;
    dy *= m_PanSensitivity;

    // AWAW TODO
    glm::vec3 lookat = m_Target - (m_Position + toCartesian());
    glm::vec3 worldUp = m_Up;
    glm::vec3 right = glm::normalize(glm::cross(lookat, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(lookat, right));

    m_Target += right * dx + up * dy;
    m_Position += right * dx + up * dy;

    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::update(float dt)
{
    (void)dt;
    updateViewMatrix();

    auto pos = toCartesian();
    auto lookDir = glm::normalize(m_Target - pos);
    m_conn.enqueue(event::UiCameraUpdate{
            .cameraPos = pos, .lookDir = lookDir, .fov = m_Fov});
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::updateViewMatrix()
{
    // TODO needs more polish
    // auto offset = toCartesian();
    // matrices.view =
    //         glm::lookAt(m_Position + offset, m_Target, m_Up);
    matrices.view = glm::lookAt(toCartesian(), m_Target, m_Up);
}

// ----------------------------------------------------------------------------
//
//

glm::vec3 TrackBall::toCartesian() const
{
    glm::vec3 ret;
    ret.x = m_Radius * std::sin(m_Phi) * std::sin(m_Theta);
    ret.y = m_Radius * std::cos(m_Phi);
    ret.z = m_Radius * std::sin(m_Phi) * std::cos(m_Theta);
    return ret;
}

} // namespace core::scene
