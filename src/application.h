#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "logs/log.h"
#include "config/config.h"

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

    VkExtent2D m_WindowResolution = {0, 0};
    VkExtent2D m_MonitorResolution = {0, 0};

    // Total runtime of application
    float m_ApprunTime = 0.0f;

    // Total time taken by one cycle of mainloop
    float m_FrameTime = 0.0f;
};

} // namespace app
