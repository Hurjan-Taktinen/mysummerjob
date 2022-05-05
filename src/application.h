#pragma once

#include "config/config.h"
#include "core/scene/camera.h"
#include "core/scene/scene.h"
#include "core/vulkan/context.h"
#include "event/sub.h"
#include "logs/log.h"
#include "ui/imguilayer.h"

#include "entt/entt.hpp"
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include <memory>

namespace app
{
class Application final : public std::enable_shared_from_this<Application>
{
public:
    Application();
    ~Application();
    void run();

private:
    void initilizeGLFW();
    void mainloop();

    void handleKeyboardInput(int key, bool isPressed, int mods);
    void handleMouseButtonInput(int button, bool isPressed);
    void handleMouseScrollInput(double yoffset);
    void handleMousePositionInput(double xpos, double ypos);
    void handleFramebufferResize(int width, int height);

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


    logs::Logger _log;
    config::BaseConfig _baseConfig;
    std::shared_ptr<GLFWwindow> _window;
    std::shared_ptr<core::vk::Context> _vulkanContext;
    std::shared_ptr<core::scene::TrackBall> _camera;
    std::shared_ptr<core::scene::Scene> _scene;
    std::unique_ptr<ui::UiLayer> _uiLayer;

    entt::registry _registry;
    entt::dispatcher _dispatcher;

    VkExtent2D _frameBufferSize = {0, 0};
    VkExtent2D _monitorResolution = {0, 0};

    // Total runtime of application
    float _apprunTime = 0.0f;

    // Total time taken by one cycle of mainloop
    float _frameTime = 0.0f;

    uint64_t _frameCounter = 0;

    // TODO move these away
    struct
    {
        glm::vec2 position = {0.0f, 0.0f};
        bool left = false;
        bool middle = false;
        bool right = false;
    } _mouse;

    struct
    {
        bool lShift = false;
    } _keyboard;
};

} // namespace app
