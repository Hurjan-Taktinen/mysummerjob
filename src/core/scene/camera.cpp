#include "core/scene/camera.h"

#include "logs/log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace core::scene
{

// ----------------------------------------------------------------------------
//
//

void TrackBall::init(uint32_t width, uint32_t height)
{
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    matrices.proj = glm::perspective(
            glm::radians(m_Fov), aspect, m_zNear, m_zFar);
    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::updateRotation(float dtheta, float dphi)
{
    dtheta *= m_MouseSensitivity;
    dphi *= m_MouseSensitivity;

    if(m_Up > 0.0f)
    {
        m_Theta += dtheta;
    }
    else
    {
        m_Theta -= dtheta;
    }

    m_Phi += dphi;

    // Clamp phi between -2PI and 2PI
    if(m_Phi > 2.0f * M_PI)
    {
        m_Phi -= 2.0f * M_PI;
    }
    else if(m_Phi < -2.0f * M_PI)
    {
        m_Phi += 2.0f * M_PI;
    }

    if((0.0f < m_Phi && m_Phi < M_PI)
       || (-M_PI > m_Phi && m_Phi > -2.0f * M_PI))
    {
        m_Up = 1.0f;
    }
    else
    {
        m_Up = -1.0f;
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
        // m_Radius = 30.0f;
        // glm::vec3 lookat = glm::normalize(m_Target - toCartesian());
        // m_Target += 30.0f * lookat;
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

    // glm::vec3 lookat = m_Target - toCartesian();
    glm::vec3 lookat = glm::normalize(m_Target - toCartesian());
    glm::vec3 worldUp = glm::vec3(0.0f, m_Up, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(lookat, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(lookat, right));

    m_Target += right * dx + up * dy;

    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::update(float dt)
{
    (void)dt;
    m_Position = toCartesian();
    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void TrackBall::updateViewMatrix()
{
    matrices.view = glm::lookAt(
            m_Position, m_Target, glm::vec3 {0.0f, m_Up, 0.0f});
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

} // namespace scene::camera
