#include "application.h"
#include "logs/log.h"
#include "timer/timer.h"

#include "event/keyevent.h"

#include "imgui/imgui_impl_glfw.h"

namespace app
{

Application::~Application()
{
    m_VulkanContext->deviceWaitIdle();
    m_Scene->clear();
    m_Scene.reset();
    ImGui_ImplGlfw_Shutdown();
    m_Log->info("Stopping");
    m_VulkanContext.reset();
    m_Camera.reset();
    m_Window.reset();
    glfwTerminate();
}

void Application::init()
{
    m_Log = logs::Log::create("Application");
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

    m_Camera = std::make_shared<core::scene::TrackBall>();
    m_Camera->init(m_FrameBufferSize.width, m_FrameBufferSize.height);

    m_Scene = std::make_unique<core::scene::Scene>(m_Camera.get());

    m_VulkanContext = std::make_shared<core::vk::Context>(
            m_Window, m_Scene.get());


    m_VulkanContext->init(m_FrameBufferSize);

    {
        // append descriptor resources to context
    }

    m_VulkanContext->generatePipelines();

    ImGui_ImplGlfw_InitForVulkan(m_Window.get(), true);

    m_Log->info("Initialization complete, entering mainloop");
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
        m_Log->critical("No Vulkan support! Exiting... ");
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
    glfwGetFramebufferSize(m_Window.get(), &width, &height);

    // If these are zero, window is minimized?
    if(width <= 0 && height <= 0)
    {
        m_Log->warn("Invalid window resolutions ({}, {})", width, height);
        throw std::runtime_error("Invalid resolution");
    }

    m_FrameBufferSize.width = static_cast<uint32_t>(width);
    m_FrameBufferSize.height = static_cast<uint32_t>(height);

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

        m_Camera->update(m_FrameTime);

        m_VulkanContext->renderFrame(m_FrameTime);

        m_FrameTime = timer.elapsed();
        m_ApprunTime += m_FrameTime;
    }
}

// -----------------------------------------------------------------------------
// GLFW Error callback
//

void Application::onErrorCallback(int error, const char* description)
{
    LGWARNING("GLFW Error ({}): {}", error, description);
}

void Application::handleKeyboardInput(int key, int action, int mods)
{
    (void)mods;
    using namespace event;

    bool isPressed = action == GLFW_PRESS;

    // This is a special case
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_Window.get(), true);
    }
    else if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
    {
        m_Keyboard.lShift = isPressed;
    }
    else if(key == GLFW_KEY_SPACE && !isPressed)
    {
        m_VulkanContext->renderImGui = !m_VulkanContext->renderImGui;
    }

    // TODO: this is how keys are handled in future
    // if(action == GLFW_PRESS)
    // {
    // event::KeyPressedEvent event {
    // KeyCode(key), KeyAction(action), KeyMods(mods)};
    // }
    // else if(action == GLFW_RELEASE)
    // {
    // event::KeyReleasedEvent event {
    // KeyCode(key), KeyAction(action), KeyMods(mods)};
    // }
}

void Application::onKeyCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard)
    {
        return;
    }

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
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        m_Mouse.left = isPressed;
    }
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        m_Mouse.middle = isPressed;
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        m_Mouse.right = isPressed;
    }
}

void Application::handleMouseScrollInput(double yoffset)
{
    m_Camera->updateZoom(yoffset);
}

void Application::handleMousePositionInput(double xpos, double ypos)
{
    const glm::vec2 newPosition(xpos, ypos);
    const glm::vec2 delta = m_Mouse.position - newPosition;
    m_Mouse.position = newPosition;

    if(m_Mouse.middle)
    {
        if(m_Keyboard.lShift)
        {
            m_Camera->pan(delta.x, delta.y);
        }
        else
        {
            m_Camera->updateRotation(delta.x, delta.y);
        }
    }
}

void Application::handleFramebufferResize(int width, int height)
{
    m_FrameBufferSize.width = static_cast<uint32_t>(width);
    m_FrameBufferSize.height = static_cast<uint32_t>(height);

    m_VulkanContext->m_FrameBufferResized = true;
}

void Application::onWindowResized(GLFWwindow* window, int width, int height)
{
    // Framebuffer resize cb is all I need
    (void)window;
    (void)width;
    (void)height;
}

void Application::onFrameBufferResized(
        GLFWwindow* window, int width, int height)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handleFramebufferResize(width, height);
}

void Application::onCursorPositionCallback(
        GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handleMousePositionInput(xpos, ypos);
}

void Application::onMouseButtonCallback(
        GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS)
    {
        app->handleMouseButtonInput(button, true);
    }
    else if(action == GLFW_RELEASE)
    {
        app->handleMouseButtonInput(button, false);
    }
}

void Application::onMouseScrollCallback(
        GLFWwindow* window, double xoffset, double yoffset)
{
    (void)xoffset;

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handleMouseScrollInput(yoffset);
}
} // namespace app
