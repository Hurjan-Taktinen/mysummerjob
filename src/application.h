#pragma once

#include "config/config.h"
#include "core/scene/camera.h"
#include "core/scene/scene.h"
#include "core/vulkan/context.h"
#include "logs/log.h"

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include <memory>

namespace app
{
class Application final
{
public:
    ~Application();
    void init();

private:
    void initilizeGLFW();
    void mainloop();

    void handleKeyboardInput(int key, int action, int mods);
    void handleMouseButtonInput(int button, bool isPressed);
    void handleMouseScrollInput(double yoffset);
    void handleMousePositionInput(double xpos, double ypos);
    void handleFramebufferResize(int  width, int height);

    // GLFW callback handlers
    static void onErrorCallback(int error, const char* description);
    static void onKeyCallback(
            GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onWindowResized(GLFWwindow* window, int width, int height);
    static void onFrameBufferResized(GLFWwindow* window, int width, int height);
    static void onCursorPositionCallback(
            GLFWwindow* window, double xpos, double ypos);
    static void onMouseButtonCallback(
            GLFWwindow* window, int button, int action, int mods);
    static void onMouseScrollCallback(
            GLFWwindow* window, double xoffset, double yoffset);

private:
    logs::Logger m_Log;
    config::BaseConfig m_BaseConfig;
    std::shared_ptr<GLFWwindow> m_Window;
    std::shared_ptr<core::vk::Context> m_VulkanContext;
    std::shared_ptr<core::scene::TrackBall> m_Camera;
    std::unique_ptr<core::scene::Scene> m_Scene;

    VkExtent2D m_FrameBufferSize = {0, 0};
    VkExtent2D m_MonitorResolution = {0, 0};

    // Total runtime of application
    float m_ApprunTime = 0.0f;

    // Total time taken by one cycle of mainloop
    float m_FrameTime = 0.0f;

    // TODO move these away
    struct
    {
        glm::vec2 position = {0.0f, 0.0f};
        bool left = false;
        bool middle = false;
        bool right = false;
    } m_Mouse;

    struct
    {
        bool lShift = false;
    } m_Keyboard;
};

} // namespace app
