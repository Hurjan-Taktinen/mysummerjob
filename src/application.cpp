#include "application.h"
#include "logs/log.h"
#include "timer/timer.h"

#include "event/keyevent.h"

namespace app
{

Application::~Application()
{
    m_Log->info("Stopping");
    m_Window.reset();
    glfwTerminate();
}

void Application::init()
{
    m_Log = logs::Log::create("application");
    m_BaseConfig = config::Config::getBaseConfig();

    try
    {
        initilizeGLFW();
    }
    catch(...)
    {
        m_Log->error("Initialization failed, exiting...");
        return;
    }

    m_Log->info("Entering mainloop");
    mainloop();
}

// -----------------------------------------------------------------------------
//
//

void Application::initilizeGLFW()
{
    glfwSetErrorCallback(onErrorCallback);
    glfwInit();

    if(glfwVulkanSupported() == GLFW_FALSE)
    {
        m_Log->warn("No Vulkan support! Exiting... ");
        throw std::runtime_error("No vulkan support");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    auto* monitor = glfwGetPrimaryMonitor();
    const auto* mode = glfwGetVideoMode(monitor);

    m_MonitorResolution.width = static_cast<uint32_t>(mode->width);
    m_MonitorResolution.height = static_cast<uint32_t>(mode->height);

    m_Log->info(
            "Display resolution ({}, {})",
            m_MonitorResolution.width,
            m_MonitorResolution.height);

    m_Window = std::shared_ptr<GLFWwindow>(
            glfwCreateWindow(
                    m_BaseConfig.width,
                    m_BaseConfig.height,
                    "MySummerJob the Game",
                    nullptr,
                    nullptr),
            [](GLFWwindow* window) { glfwDestroyWindow(window); });

    assert(m_Window);

    int width = 0;
    int height = 0;
    glfwGetWindowSize(m_Window.get(), &width, &height);

    // If these are zero, window is minimized?
    if(width <= 0 && height <= 0)
    {
        m_Log->warn("Invalid window resolutions ({}, {})", width, height);
        throw std::runtime_error("Invalid resolution");
    }

    m_WindowResolution.width = static_cast<uint32_t>(width);
    m_WindowResolution.height = static_cast<uint32_t>(height);

    m_Log->info(
            "GLFW Window surface created with size of ({}, {})", width, height);

    glfwSetWindowUserPointer(m_Window.get(), this);
    glfwSetKeyCallback(m_Window.get(), onKeyCallback);
    glfwSetFramebufferSizeCallback(m_Window.get(), onFrameBufferResized);
    glfwSetCursorPosCallback(m_Window.get(), onCursorPositionCallback);
    glfwSetScrollCallback(m_Window.get(), onMouseScrollCallback);
    glfwSetMouseButtonCallback(m_Window.get(), onMouseButtonCallback);
}

// -----------------------------------------------------------------------------
// Mainloop of the application, everything should be synced to this and it
// should be running at framerate of the monitor
//

void Application::mainloop()
{
    while(!glfwWindowShouldClose(m_Window.get()))
    {
        Timer timer;
        glfwPollEvents();

        m_FrameTime = timer.elapsed();
        m_ApprunTime += m_FrameTime;
    }
}

// -----------------------------------------------------------------------------
// GLFW Error callback
//

void Application::onErrorCallback(int error, const char* description)
{
    GLWARNING("GLFW Error ({}): {}", error, description);
}

void Application::handleKeyboardInput(int key, int action, int mods)
{
    GLINFO("key({}) isPressed({})", key, action == GLFW_PRESS);
    using namespace event;

    // This is a special case
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_Window.get(), true);
    }

    if(action == GLFW_PRESS)
    {
        event::KeyPressedEvent event {
                KeyCode(key), KeyAction(action), KeyMods(mods)};

    }
    else if(action == GLFW_RELEASE)
    {
        event::KeyReleasedEvent event {
                KeyCode(key), KeyAction(action), KeyMods(mods)};
    }
}

void Application::onKeyCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;

    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS)
    {
        app->handleKeyboardInput(key, action, mods);
    }
    else if(action == GLFW_RELEASE)
    {
        app->handleKeyboardInput(key, action, mods);
    }
}

void Application::handleMouseButtonInput(int button, bool isPressed)
{
    GLINFO("Button({}) isPressed({})", button, isPressed);
}

void Application::handleMouseScrollInput(double yoffset)
{
    (void)yoffset;
}

void Application::handleMousePositionInput(double xpos, double ypos)
{
    (void)xpos;
    (void)ypos;
}

void Application::onWindowResized(GLFWwindow* window, int width, int height)
{
    (void)window;
    (void)width;
    (void)height;
}

void Application::onFrameBufferResized(
        GLFWwindow* window, int width, int height)
{
    (void)window;
    GLINFO("New framebuffer size ({}, {})", width, height);
}

void Application::onCursorPositionCallback(
        GLFWwindow* window, double xpos, double ypos)
{
    (void)window;
    (void)xpos;
    (void)ypos;
}

void Application::onMouseButtonCallback(
        GLFWwindow* window, int button, int action, int mods)
{
    (void)window;
    (void)button;
    (void)action;
    (void)mods;
}

void Application::onMouseScrollCallback(
        GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    (void)yoffset;
}
} // namespace app
