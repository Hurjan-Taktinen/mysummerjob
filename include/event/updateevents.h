#pragma once

#include <glm/vec3.hpp>

namespace event
{
struct UiCameraUpdate
{
    glm::vec3 cameraPos;
    glm::vec3 lookDir;
    float fov = 0.0f;
};
} // namespace event
